#!/usr/bin/env python3
"""Offline regressions for the security auditor's LLVM function index."""

import os
import io
import contextlib
import sys
import unittest
from unittest.mock import mock_open, patch

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "security"))
import codetree


class CodeTreeTests(unittest.TestCase):
    def test_operator_metadata_retains_header_location(self):
        ir = ('define void @callback() !dbg !1 {\n  ret void\n}\n'
              '!1 = distinct !DISubprogram(name: "operator()(int)", '
              'file: !2, line: 17, scopeLine: 18)\n'
              '!2 = !DIFile(filename: "callback.hpp", directory: "/project")\n')
        funcs, _ = codetree.parse_function_defs(ir, "/project/main.cpp")
        self.assertEqual([(f.file, f.line) for f in funcs],
                         [("/project/callback.hpp", 17)])

    def test_undemangled_std_qualifiers_are_not_review_targets(self):
        symbols = ("_ZSt4move", "_ZNSt6ranges4move", "_ZNKSt6ranges4move",
                   "_ZNVKSt6ranges4move", "_ZNKRSt6ranges4move",
                   "_ZNOSt6ranges4move")
        for symbol in symbols:
            with self.subTest(symbol=symbol), patch.object(
                    codetree, "_demangle_many", side_effect=lambda names: {n:n for n in names}):
                ir = ('define void @handler() !dbg !1 {\n'
                      '  call void @' + symbol + '()\n  ret void\n}\n'
                      'define void @' + symbol + '() {\n'
                      '  call void @callback()\n  ret void\n}\n'
                      '!1 = distinct !DISubprogram(name: "handler", line: 1)\n')
                funcs, calls = codetree.parse_function_defs(ir, "/project/main.cpp")
                self.assertEqual([f.qual_name for f in funcs], ["handler"])
                self.assertEqual(calls, [])

    def test_std_arguments_do_not_exclude_project_function(self):
        symbol = "_ZNK6Server6handleERKSt6vectorIiSaIiEE"
        with patch.object(codetree, "_demangle_many", side_effect=lambda names: {n:n for n in names}):
            funcs, _ = codetree.parse_function_defs(
                'define void @' + symbol + '() {\n  ret void\n}\n',
                "/project/main.cpp")
        self.assertEqual([f.name for f in funcs], [symbol])

    def test_excluded_definition_calls_never_belong_to_previous_function(self):
        names = {"library": "std::helper", "thunk": "virtual thunk to Handler::run"}
        for name in names:
            with self.subTest(name=name), patch.object(
                    codetree, "_demangle_many", side_effect=lambda symbols: {
                        n: names.get(n, n) for n in symbols}):
                ir = ('define void @handler() {\n  call void @real_target()\n'
                      '  ret void\n}\n'
                      'define void @' + name + '() {\n'
                      '  call void @false_target()\n  ret void\n}\n')
                funcs, calls = codetree.parse_function_defs(ir, "/project/main.cpp")
                self.assertEqual([f.qual_name for f in funcs], ["handler"])
                self.assertEqual(calls, [("handler", "real_target", 0)])

    def test_definition_attributes(self):
        for attributes in ("local_unnamed_addr #0 align 2", "unnamed_addr #0",
                           "#0", "align 2", ""):
            with self.subTest(attributes=attributes):
                ir = ('define void @handler(ptr initializes((0, 48)) %p) '
                      + attributes + ' !dbg !1 {\n  ret void\n}\n'
                      '!1 = distinct !DISubprogram(name: "handler", line: 7)\n')
                funcs, calls = codetree.parse_function_defs(ir, "/project/handler.cpp")
                self.assertEqual([(f.qual_name, f.line) for f in funcs],
                                 [("handler", 7)])
                self.assertEqual(calls, [])

    def test_calls_with_return_attributes_and_invoke(self):
        for instruction in ('call noundef nonnull align 8 ptr',
                            'tail call fastcc zeroext i8', 'invoke noundef i32'):
            with self.subTest(instruction=instruction):
                ir = ('define void @handler() {\n  %1 = ' + instruction
                      + ' @target(ptr %p)\n  ret void\n}\n')
                _, calls = codetree.parse_function_defs(ir, '/project/main.cpp')
                self.assertEqual(calls, [('handler', 'target', 0)])

    def test_overloads_and_local_symbols_stay_distinct(self):
        names = {'_Z1fi':'f(int)', '_Z1fPc':'f(char*)', '_ZL6helperv':'helper()'}
        with patch.object(codetree, '_demangle_many', side_effect=lambda symbols: {
                n:names.get(n,n) for n in symbols}):
            ir = ('define void @_Z1fi() {\n  ret void\n}\n'
                  'define void @_Z1fPc() {\n  call void @_Z1fi()\n  ret void\n}\n'
                  'define internal void @_ZL6helperv() {\n  ret void\n}\n')
            first, calls = codetree.parse_function_defs(ir, '/project/a.cpp')
            second, _ = codetree.parse_function_defs(ir, '/project/b.cpp')
        self.assertEqual([f.qual_name for f in first[:2]], ['f(int)', 'f(char*)'])
        self.assertEqual(calls, [('f(char*)', 'f(int)', 0)])
        self.assertNotEqual(first[2].qual_name, second[2].qual_name)

    def test_demangling_is_batched_and_preserves_signatures(self):
        with patch.object(codetree.subprocess, 'run') as run:
            run.return_value.returncode = 0
            run.return_value.stdout = 'f(char*)\nf(int)\n'
            result = codetree._demangle_many(['_Z1fi', '_Z1fPc', '_Z1fi', 'plain'])
        run.assert_called_once()
        self.assertNotIn('-p', run.call_args.args[0])
        self.assertEqual(result, {'_Z1fi':'f(int)', '_Z1fPc':'f(char*)', 'plain':'plain'})

    def test_source_body_skips_initializers_and_literal_braces(self):
        sources = (
            'X::X(): first{42}, second{7}\n{\n    dangerous();\n}\n',
            'void f(int n=[]{return 2;}())\n{ sink(n); }\n',
            'void f() {\n const char *s=R"tag(" } { )tag";\n sink();\n}\n',
            'void f() {\n /* } */ char c=\'}\'; // }\n sink();\n}\n')
        for source in sources:
            with self.subTest(source=source), patch('builtins.open', mock_open(read_data=source)):
                self.assertEqual(codetree.source_body('/project/main.cpp',1)[0],source)

    def test_inherited_class_body_remains_recognizable_as_generated(self):
        source='class Timer : public BaseTimer\n{\n void exec();\n};\n'
        with patch('builtins.open',mock_open(read_data=source)):
            self.assertEqual(codetree.source_body('/project/main.cpp',1)[0],source)

    def test_inline_lambda_body_starts_inside_a_call(self):
        source='std::sort(v.begin(),v.end(),[](int a,int b) {\n return a<b;\n});\n'
        fi=codetree.FuncInfo('lambda','lambda','/project/main.cpp',1,0,kind='LambdaExpr')
        with patch('builtins.open',mock_open(read_data=source)):
            self.assertEqual(codetree.function_body(fi),(source,3))

    def test_conditional_alternative_braces_do_not_accumulate(self):
        source=('int f() {\n#if FIRST\n if(first()) {\n#elif SECOND\n'
                ' if(second()) {\n#else\n if(third()) {\n#endif\n'
                '  return 1;\n }\n return 0;\n}\n')
        with patch('builtins.open',mock_open(read_data=source)):
            self.assertEqual(codetree.source_body('/project/main.cpp',1)[0],source)

    def test_compiler_initializers_are_not_source_functions(self):
        for name in ('__cxx_global_var_init.1','__cxx_global_array_dtor',
                     '_GLOBAL__sub_I_main.cpp'):
            funcs,_=codetree.parse_function_defs(
                'define internal void @'+name+'() {\n ret void\n}\n','/project/main.cpp')
            self.assertEqual(funcs,[])

    def test_same_signature_in_different_binaries_keeps_both_bodies(self):
        idx=codetree.Index()
        login=codetree.FuncInfo('handler','handler(int)','/project/login.cpp',1,3)
        gateway=codetree.FuncInfo('handler','handler(int)','/project/gateway.cpp',7,9)
        caller=codetree.FuncInfo('caller','caller()','/project/caller.cpp',1,3)
        with patch.object(codetree,'REPO_ROOT','/project'):
            idx._merge_definitions([
                ('login.cpp',[login],[('handler(int)','loginGuard()',2)]),
                ('gateway.cpp',[gateway],[('handler(int)','gatewayGuard()',8)]),
                ('caller.cpp',[caller],[('caller()','handler(int)',2)])])
        self.assertEqual(len(idx.by_name),3)
        self.assertNotEqual(login.qual_name,gateway.qual_name)
        self.assertEqual(idx._forward_callees[login.qual_name],{'loginGuard()':[2]})
        self.assertEqual(idx._forward_callees[gateway.qual_name],{'gatewayGuard()':[8]})
        self.assertEqual(set(idx._forward_callees[caller.qual_name]),
                         {login.qual_name,gateway.qual_name})

    def test_same_header_definition_is_deduplicated(self):
        idx=codetree.Index()
        first=codetree.FuncInfo('helper','helper()','/project/helper.hpp',1,3)
        second=codetree.FuncInfo('helper','helper()','/project/helper.hpp',1,3)
        with patch.object(codetree,'REPO_ROOT','/project'):
            idx._merge_definitions([
                ('first.cpp',[first],[('helper()','sink()',2)]),
                ('second.cpp',[second],[('helper()','sink()',2)])])
        self.assertEqual(len(idx.by_name),1)
        self.assertEqual(idx._forward_callees,{'helper()':{'sink()':[2]}})

    def test_declaration_never_steals_next_function_body(self):
        with patch('builtins.open', mock_open(read_data='void f();\nvoid g() { sink(); }\n')):
            self.assertEqual(codetree.source_body('/project/main.cpp',1), ('',1))

    def test_tree_depth_is_enforced(self):
        idx = codetree.Index()
        idx._built = True
        idx._forward_callees = {'a':{'b':[1]}, 'b':{'c':[2]}}
        idx._reverse_callers = {'a':{'b':[1]}, 'b':{'c':[2]}}
        for render in (codetree.TreeRender.caller_tree, codetree.TreeRender.callee_tree):
            tree=render(idx,'a',depth=1)
            self.assertIn('b  [lines',tree)
            self.assertNotIn('c  [lines',tree)

    def test_small_compile_scope_does_not_include_unbuilt_files(self):
        with patch.object(codetree,'SCOPE_DIRS',('/project',)), \
                patch.object(codetree,'_BUILD_CDB_ONLY',True), \
                patch.object(codetree,'_load_cdb',return_value={'/project/yes.c':'clang'}), \
                patch.object(codetree.os.path,'isdir',return_value=True), \
                patch.object(codetree.os,'walk',return_value=[('/project',[],['yes.c','no.cpp'])]):
            self.assertEqual(codetree.Index()._collect_sources(),['/project/yes.c'])

    def test_header_contents_invalidate_source_stamp(self):
        with patch.object(codetree,'_SOURCE_STAMP',''), \
                patch.object(codetree.subprocess,'run') as run:
            run.return_value.stdout=b'general/guard.hpp\0'
            with patch('builtins.open',mock_open(read_data=b'old guard')):
                first=codetree.refresh_source_stamp()
            with patch('builtins.open',mock_open(read_data=b'new guard')):
                second=codetree.refresh_source_stamp()
        self.assertNotEqual(first,second)

    def test_worker_exception_is_a_coverage_failure(self):
        idx=codetree.Index()
        with patch.object(idx,'_collect_sources',return_value=['/project/main.cpp']), \
                patch.object(codetree,'_ir_cached',side_effect=ValueError('invalid IR')), \
                contextlib.redirect_stderr(io.StringIO()):
            idx.build(max_workers=1)
        self.assertEqual(idx.errors,[('/project/main.cpp','worker error: invalid IR')])

    def test_call_belongs_to_local_unnamed_definition(self):
        ir = ('define void @previous() #0 !dbg !1 {\n  ret void\n}\n'
              'define void @handler() local_unnamed_addr #0 !dbg !2 {\n'
              '  call void @target(), !dbg !3\n  ret void\n}\n'
              'declare void @target()\n'
              '!1 = distinct !DISubprogram(name: "previous", line: 1)\n'
              '!2 = distinct !DISubprogram(name: "handler", line: 8)\n'
              '!3 = !DILocation(line: 9, column: 1, scope: !2)\n')
        funcs, calls = codetree.parse_function_defs(ir, "/project/handler.cpp")
        self.assertEqual([f.qual_name for f in funcs], ["previous", "handler"])
        self.assertEqual(calls, [("handler", "target", 9)])

    def test_index_flags_override_release_flags(self):
        with patch.object(codetree, "CLANG", "clang"), \
                patch.object(codetree, "flags_for", return_value="-O3 -g0"), \
                patch.object(codetree.subprocess, "run") as run:
            run.return_value.returncode = 0
            run.return_value.stdout = "IR"
            self.assertEqual(codetree.ir_for("handler.cpp"), ("IR", ""))
            command = run.call_args.args[0]
            self.assertEqual([arg for arg in command if arg.startswith("-O")][-1],
                             "-O0")
            self.assertEqual([arg for arg in command if arg.startswith("-g")][-1],
                             "-g")

    def test_cache_distinguishes_paths_flags_and_legacy_ir(self):
        with patch.object(codetree.os.path, "getmtime", return_value=1.0), \
                patch.object(codetree, "flags_for", side_effect=["-DA", "-DB", "-DB"]), \
                patch("builtins.open", mock_open(read_data="IR")) as opened:
            names = []
            for path in ("/project/a/handler.cpp", "/project/a/handler.cpp",
                         "/project/b/handler.cpp"):
                self.assertEqual(codetree._ir_cached(path), ("IR", ""))
                names.append(opened.call_args.args[0])
            self.assertEqual(len(set(names)), 3)
            legacy = os.path.join(codetree._IR_CACHE_DIR, "handler.cpp.1x000000000.ir")
            self.assertNotIn(legacy, names)


def run_tests():
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(CodeTreeTests)
    return unittest.TextTestRunner(failfast=True).run(suite).wasSuccessful()


if __name__ == "__main__":
    sys.exit(0 if run_tests() else 1)
