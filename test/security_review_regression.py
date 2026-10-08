#!/usr/bin/env python3
"""Offline review regressions: coverage, evidence access, and incomplete verdicts."""

import contextlib
import functools
import io
import json
import os
import subprocess
import sys
import tempfile
import time
import unittest
from types import SimpleNamespace
from unittest.mock import mock_open, patch

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "security"))
import agentic
import codetree
import server

codecheck = agentic.codecheck
FINDING = "SEVERITY(HIGH) | handler.cpp:3 | client length reaches an unchecked copy"


class SecurityReviewTests(unittest.TestCase):
    def setUp(self):
        self.fi = codetree.FuncInfo("handler", "handler", "/project/handler.cpp", 1, 3)
        agentic.common._set_truncated(False)
        agentic.common._CANCEL_REQUESTED = False

    def tearDown(self):
        agentic.common._set_truncated(False)
        agentic.common._CHAT_OBSERVER.callback = None
        agentic.common._CANCEL_REQUESTED = False

    def test_interrupt_dumps_workers_then_second_interrupt_forces_exit(self):
        with patch.object(server.signal, 'signal') as install, \
                patch.object(server.faulthandler, 'enable'), \
                patch.object(server.faulthandler, 'register'), \
                patch.object(server.faulthandler, 'dump_traceback') as dump, \
                patch.object(server.os, 'write') as write, \
                patch.object(server.os, '_exit', side_effect=SystemExit(130)) as force_exit, \
                contextlib.redirect_stderr(io.StringIO()):
            server._install_backtrace_signals()
            handler = install.call_args.args[1]
            with self.assertRaises(KeyboardInterrupt):
                handler(server.signal.SIGINT, None)
            dump.assert_called_once_with(file=2, all_threads=True)
            self.assertTrue(agentic.common._CANCEL_REQUESTED)
            self.assertIn(b'before shutdown', write.call_args_list[0].args[1])
            force_exit.assert_not_called()
            self.assertEqual(install.call_args.args, (server.signal.SIGINT, handler))
            with self.assertRaises(SystemExit) as stopped:
                handler(server.signal.SIGINT, None)
            self.assertEqual(stopped.exception.code, 130)
            force_exit.assert_called_once_with(130)
            self.assertEqual(dump.call_count, 2)
            self.assertIn(b'shutdown in progress', write.call_args_list[-2].args[1])

    def test_forced_exit_survives_diagnostic_failure(self):
        with patch.object(server.signal, 'signal') as install, \
                patch.object(server.faulthandler, 'enable'), \
                patch.object(server.faulthandler, 'register'), \
                patch.object(server.faulthandler, 'dump_traceback'), \
                patch.object(server.os, 'write') as write, \
                patch.object(server.os, '_exit', side_effect=SystemExit(130)), \
                contextlib.redirect_stderr(io.StringIO()):
            server._install_backtrace_signals()
            handler = install.call_args.args[1]
            with self.assertRaises(KeyboardInterrupt):
                handler(server.signal.SIGINT, None)
            write.side_effect = OSError('stderr closed')
            with self.assertRaises(SystemExit) as stopped:
                handler(server.signal.SIGINT, None)
            self.assertEqual(stopped.exception.code, 130)

    def test_cancelled_review_does_not_send_another_model_request(self):
        agentic.common.request_cancel()
        with patch.object(agentic.common, 'chat_with') as chat:
            with self.assertRaises(agentic.common.CancelledError):
                agentic._chat(None, [], time.time() + 60)
            chat.assert_not_called()

    def test_cancelled_inflight_reply_is_not_accepted(self):
        def reply(*args, **kwargs):
            agentic.common.request_cancel()
            return 'NO ISSUES'
        with patch.object(agentic.common, 'chat_with', side_effect=reply):
            with self.assertRaises(agentic.common.CancelledError):
                agentic._chat(None, [], time.time() + 60)

    def test_non_stopping_backtrace_uses_native_signal_handler(self):
        with patch.object(server.signal, 'signal'), \
                patch.object(server.faulthandler, 'enable') as enable, \
                patch.object(server.faulthandler, 'register') as register, \
                contextlib.redirect_stderr(io.StringIO()):
            server._install_backtrace_signals()
        enable.assert_called_once_with(file=2, all_threads=True)
        if hasattr(server.signal, 'SIGUSR1'):
            register.assert_called_once_with(server.signal.SIGUSR1,
                                             file=2, all_threads=True, chain=False)

    def test_cli_interrupt_returns_130_without_another_traceback(self):
        with patch.object(server, '_install_backtrace_signals'), \
                patch.object(server, 'main', side_effect=KeyboardInterrupt), \
                patch.object(server.os, 'write'):
            self.assertEqual(server._run_cli(['server.py']), 130)

    def test_sandbox_environment_does_not_inherit_credentials_or_loader_hooks(self):
        with patch.dict(os.environ, {'SSH_AUTH_SOCK': '/private/agent',
                                     'AWS_SECRET_ACCESS_KEY': 'fixture-secret',
                                     'LD_PRELOAD': '/private/inject.so',
                                     'PYTHONPATH': '/private/modules'}):
            env = server._sandbox_env()
        self.assertEqual(set(env), {'PATH', 'HOME', 'TMPDIR', 'LANG', 'LC_ALL'})
        self.assertNotIn('fixture-secret', env.values())
        self.assertEqual(env['HOME'], '/tmp')

    def test_sandbox_mounts_only_runtime_and_uses_private_processes(self):
        with patch.object(server, 'BWRAP', '/usr/bin/bwrap'), \
                patch.object(server.os.path, 'exists', return_value=True):
            args = server._sandbox_base()
        for flag in ('--unshare-user', '--unshare-pid', '--unshare-ipc',
                     '--disable-userns', '--die-with-parent', '--new-session'):
            self.assertIn(flag, args)
        self.assertEqual(args[args.index('--cap-drop') + 1], 'ALL')
        mounts = [args[i + 1] for i, arg in enumerate(args) if arg == '--ro-bind']
        self.assertEqual(mounts, ['/usr', '/bin', '/sbin', '/lib', '/lib64', '/etc/ld.so.cache'])
        self.assertIn('--unshare-net', args)

    def test_private_network_wrap_joins_only_pinned_namespace_fds(self):
        network = server.SandboxNetwork()
        network.proc = SimpleNamespace(poll=lambda: None)
        network.fds = (7, 8)
        command = network.wrap(['/usr/bin/bwrap', '--unshare-net', '--unshare-pid',
                                '--cap-drop', 'ALL', '/fixture'])
        self.assertEqual(command[0], '/usr/bin/bwrap')
        self.assertNotIn('--unshare-net', command)
        self.assertIn('--unshare-pid', command)
        self.assertEqual(command[command.index('--cap-drop') + 1], 'ALL')

    def test_missing_private_network_never_runs_on_host(self):
        network = server.SandboxNetwork()
        with self.assertRaisesRegex(RuntimeError, 'refusing host-network fallback'):
            network.wrap(['bwrap', '--unshare-net'])
        with patch.object(server, '_compile_exploit') as compile_exploit:
            self.assertIn('private target network required', server.do_run('/fixture'))
            compile_exploit.assert_not_called()
        with patch.object(server.os, 'setns', None), patch.object(server.os, 'pipe') as pipe:
            with self.assertRaisesRegex(RuntimeError, 'os.setns required'):
                network.start()
            pipe.assert_not_called()

    def test_child_joins_private_network_then_closes_privileged_handles(self):
        network = server.SandboxNetwork()
        network.fds = (7, 8)
        events = []
        with patch.object(server.os, 'setns', side_effect=lambda fd, kind: events.append((fd, kind))), \
                patch.object(server.os, 'close', side_effect=lambda fd: events.append(('close', fd))):
            network.enter(lambda: events.append('limits'))
        self.assertEqual(events, [(7, os.CLONE_NEWUSER), (8, os.CLONE_NEWNET),
                                  ('close', 7), ('close', 8), 'limits'])

    def test_network_setup_rejects_host_namespace_or_changed_identity(self):
        import fcntl
        import select
        for actual, host in ((456, 456), (999, 111)):
            network = server.SandboxNetwork()
            with self.subTest(actual=actual, host=host), \
                    patch.object(server, '_sandbox_base', return_value=['bwrap', '--disable-userns']), \
                    patch.object(server.os, 'pipe', return_value=(10, 11)), \
                    patch.object(server.os, 'close') as close, \
                    patch.object(server.os, 'read', return_value=b'{"child-pid":123,"net-namespace":456}'), \
                    patch.object(server.os, 'open', return_value=12), \
                    patch.object(server.os, 'fstat', return_value=SimpleNamespace(st_ino=actual)), \
                    patch.object(server.os, 'stat', return_value=SimpleNamespace(st_ino=host)), \
                    patch.object(select, 'select', return_value=([10], [], [])), \
                    patch.object(fcntl, 'ioctl') as ioctl, \
                    patch.object(server.subprocess, 'Popen') as spawn:
                spawn.return_value.communicate.return_value = (None, b'')
                with self.assertRaisesRegex(RuntimeError, 'identity mismatch'):
                    network.start()
                ioctl.assert_not_called()
                spawn.return_value.kill.assert_called_once()
                self.assertEqual(sorted(c.args[0] for c in close.call_args_list), [10, 11, 12])
                self.assertEqual(network.fds, ())

    def test_health_probe_setup_failure_is_not_a_confirmed_server_hang(self):
        live = server.LiveServer('/fixture')
        with patch.object(live.network, 'wrap', side_effect=lambda cmd: cmd), \
                patch.object(server.subprocess, 'run') as run:
            run.return_value.returncode = 1
            run.return_value.stderr = 'namespace unavailable'
            with self.assertRaisesRegex(RuntimeError, 'health probe failed'):
                live._protocol_ping()
        self.assertFalse(live.hung)

    def test_health_probe_executes_in_private_network_and_validates_result(self):
        live = server.LiveServer('/fixture')
        live.network.fds = (7, 8)
        with patch.object(live.network, 'wrap', side_effect=lambda cmd: ['private-network'] + cmd) as wrap, \
                patch.object(server.subprocess, 'run') as run:
            run.return_value.returncode = 0
            run.return_value.stdout = '[true, false]'
            self.assertEqual(live._protocol_ping(), (True, False))
            self.assertEqual(run.call_args.args[0][0], 'private-network')
            self.assertEqual(run.call_args.kwargs['pass_fds'], (7, 8))
            wrap.assert_called_once()
            run.return_value.stdout = '["not a boolean", false]'
            with self.assertRaisesRegex(RuntimeError, 'invalid private network'):
                live._protocol_ping()

    def test_debugger_never_replays_unsandboxed_when_bwrap_is_missing(self):
        with patch.object(server, 'BWRAP', None), \
                patch.object(server.os.path, 'isfile', return_value=True), \
                patch.object(server.shutil, 'which', return_value='/usr/bin/gdb'), \
                patch.object(server.subprocess, 'run') as run:
            self.assertIn('refusing unsandboxed', server._exploit_backtrace('/fixture/exploit'))
            run.assert_not_called()

    def test_exploit_compilation_is_sandboxed_without_host_environment(self):
        with patch.object(server, 'BWRAP', '/usr/bin/bwrap'), \
                patch.object(server.os, 'listdir', return_value=['attack.cpp']), \
                patch.object(server.os.path, 'isfile', return_value=True), \
                patch.object(server.subprocess, 'run') as run:
            run.return_value.returncode = 0
            binary, error = server._compile_exploit('/fixture')
        self.assertIsNone(error)
        self.assertEqual(binary, '/fixture/' + server.EXPLOIT_BIN_NAME)
        command = run.call_args.args[0]
        self.assertEqual(command[0], '/usr/bin/bwrap')
        self.assertIn('--unshare-net', command)
        self.assertEqual(run.call_args.kwargs['env'], server._sandbox_env())

    def test_model_cannot_write_target_state_or_sandbox_filter(self):
        with patch('builtins.open') as opened:
            for path in ('gdb-run/database', './gdb-run/database',
                         '.seccomp.bpf', './.seccomp.bpf'):
                with self.subTest(path=path):
                    self.assertIn('reserved', server.do_write('/fixture', path, 'fixture'))
            opened.assert_not_called()

    def test_target_pid_translation_uses_only_sandbox_descendants(self):
        files = {'/proc/100/status': 'NSpid:\t100\n',
                 '/proc/100/task/100/children': '200',
                 '/proc/200/status': 'NSpid:\t200\t1\n',
                 '/proc/200/task/200/children': '300',
                 '/proc/300/status': 'NSpid:\t300\t2\n'}
        live = server.LiveServer('/fixture')
        live.proc = SimpleNamespace(pid=100)
        with patch('builtins.open', side_effect=lambda path: io.StringIO(files[path])):
            self.assertEqual(live._host_pid(2), 300)

    def test_restart_rebuilds_state_without_copying_old_database(self):
        import tempfile
        from pathlib import Path
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            binary = root / 'server'
            binary.write_text('fixture binary')
            kit = root / 'kit'
            (kit / 'datapack').mkdir(parents=True)
            (kit / 'stale-database').write_text('must not be copied')
            output = root / 'output'
            output.mkdir()
            live = server.LiveServer(str(output))
            with patch.object(server, 'SERVER_BIN', str(binary)), \
                    patch.object(server, 'STAGED_RUN', str(kit)), \
                    contextlib.redirect_stderr(io.StringIO()):
                live._reset_run_state()
                run = Path(live.rundir)
                (run / 'database').write_text('mutated game state')
                live._tail.append('previous server output')
                live._reset_run_state()
            self.assertEqual(sorted(p.name for p in run.iterdir()),
                             ['catchchallenger-server-cli', 'datapack', 'server-properties.xml'])
            self.assertEqual((run / 'server-properties.xml').read_text(),
                             server.STAGED_PROPERTIES.format(port=server.GDB_PORT))
            self.assertEqual(list(live._tail), [])
            self.assertEqual((kit / 'stale-database').read_text(), 'must not be copied')

    def test_reset_refuses_a_symlink_to_an_external_directory(self):
        live = server.LiveServer('/fixture')
        with patch.object(server.os.path, 'islink', return_value=True), \
                patch.object(server.shutil, 'rmtree') as delete:
            with self.assertRaisesRegex(OSError, 'symlink'):
                live._reset_run_state()
            delete.assert_not_called()

    def test_stop_waits_for_target_exit_before_releasing_state(self):
        import select
        for exits in (True, False):
            live = server.LiveServer('/fixture')
            live._pid = 300
            live.mfd = 10
            events = []
            live.proc = SimpleNamespace(kill=lambda: events.append('kill'),
                                        wait=lambda timeout: events.append('wait'))
            with self.subTest(exits=exits), \
                    patch.object(live, '_w'), \
                    patch.object(server.os, 'pidfd_open', return_value=9) as pin, \
                    patch.object(server.os, 'close') as close, \
                    patch.object(select, 'poll') as poll:
                poll.return_value.poll.return_value = [(9, select.POLLIN)] if exits else []
                if exits:
                    live.stop()
                    self.assertIsNone(live.proc)
                else:
                    with self.assertRaisesRegex(RuntimeError, 'refusing to reuse'):
                        live.stop()
                    self.assertIsNotNone(live.proc)
                pin.assert_called_once_with(300)
                poll.return_value.register.assert_called_once_with(9, select.POLLIN)
                self.assertEqual(events, ['kill', 'wait'])
                self.assertEqual([call.args[0] for call in close.call_args_list], [9, 10])
                self.assertEqual(live.mfd, -1)

    def test_progress_restores_stderr_before_an_interruptible_join(self):
        output = io.StringIO()
        output.isatty = lambda: True
        with contextlib.redirect_stderr(output), patch.object(server.threading, 'Thread') as thread:
            progress = server._AuditProgress()
            progress.__enter__()
            thread.return_value.join.side_effect = KeyboardInterrupt
            with self.assertRaises(KeyboardInterrupt):
                progress.stop()
            self.assertIs(sys.stderr, output)

    def test_progress_rewrites_one_terminal_line(self):
        output = io.StringIO()
        output.isatty = lambda: True
        with contextlib.redirect_stderr(output), \
                patch.object(server.shutil, 'get_terminal_size', return_value=os.terminal_size((240, 24))):
            progress = server._AuditProgress()
            progress.set_phase('Review', 0, 2)
            progress.activity('request')
            progress.activity('tokens', 5)
            progress.activity('tokens', 5)  # final usage repeats cumulative timings
            progress.activity('tokens', 7)
            progress.activity('output', 20)
            progress.complete(True, False, 12)
            progress.render()
            progress.render()
        text = output.getvalue()
        self.assertEqual(text.count('\r\033[2K'), 2)
        self.assertNotIn('\n', text)
        self.assertIn('[Review] 1/2', text)
        self.assertIn('tok 7', text)
        self.assertIn('lines 12', text)
        self.assertIn('idle 0s', text)

    def test_progress_unknown_tokens_are_not_guessed_from_chunks(self):
        output = io.StringIO()
        output.isatty = lambda: True
        with contextlib.redirect_stderr(output):
            progress = server._AuditProgress()
            progress.set_phase('Review', 0, 2)
            progress.activity('request')
            progress.activity('output', 50)
            progress.render()
        self.assertIn('tok ?', output.getvalue())
        self.assertEqual(progress.chars, 50)

    def test_progress_counts_failed_functions_and_does_not_spam_logs(self):
        output = io.StringIO()
        with contextlib.redirect_stderr(output):
            progress = server._AuditProgress()
            progress.set_phase('Review', 0, 2)
            progress.render()
            progress.render()
            progress.complete(False, True, 3)
            progress.complete(False, False, 4)
        self.assertEqual(progress.done, 2)
        self.assertEqual(progress.errors, 1)
        self.assertEqual(progress.lines, 7)
        self.assertNotIn('\r', output.getvalue())
        self.assertEqual(len(output.getvalue().splitlines()), 2)

    def test_progress_tick_waits_one_second(self):
        with contextlib.redirect_stderr(io.StringIO()):
            progress = server._AuditProgress()
        with patch.object(progress.stopped, 'wait', side_effect=[False, True]) as wait, \
                patch.object(progress, 'render') as render:
            progress._tick()
        self.assertEqual(wait.call_args_list[0].args, (1,))
        render.assert_called_once()

    def test_progress_preserves_messages_written_in_multiple_parts(self):
        output = io.StringIO()
        output.isatty = lambda: True
        with contextlib.redirect_stderr(output):
            progress = server._AuditProgress()
            progress.render()
            progress.write('diagnostic message')
            progress.render()  # do not overwrite a partially written message
            progress.write('\n')
        self.assertTrue(output.getvalue().endswith('diagnostic message\n'))

    def test_progress_restores_stderr_and_stops_thread_on_error(self):
        output = io.StringIO()
        output.isatty = lambda: True
        with contextlib.redirect_stderr(output), patch.object(server.threading, 'Thread') as thread:
            with self.assertRaisesRegex(ValueError, 'fixture'):
                with server._AuditProgress():
                    raise ValueError('fixture')
            self.assertIs(sys.stderr, output)
        thread.return_value.join.assert_called_once()

    def test_llamacpp_reports_real_tokens_and_reasoning_activity(self):
        common = agentic.common
        events = []
        common._CHAT_OBSERVER.callback = lambda event, size: events.append((event, size))
        chunks = [
            {'choices': [{'delta': {'reasoning_content': 'abc'}}], 'timings': {'predicted_n': 2}},
            {'choices': [{'delta': {'content': 'NO ISSUES'}, 'finish_reason': 'stop'}]},
            {'choices': [], 'usage': {'completion_tokens': 3}}]
        with patch.object(common.urllib.request, 'urlopen') as opened, \
                patch.object(common, 'backend_for_model', return_value='http://backend.invalid'):
            opened.return_value.__enter__.return_value = [
                ('data: ' + json.dumps(c) + '\n').encode() for c in chunks]
            self.assertEqual(common._chat_llamacpp([], model='fixture'), 'NO ISSUES')
        payload = json.loads(opened.call_args.args[0].data)
        self.assertTrue(payload['timings_per_token'])
        self.assertTrue(payload['stream_options']['include_usage'])
        self.assertNotIn('repeat_penalty', payload)
        self.assertEqual(events, [('request', 0), ('tokens', 2), ('output', 3),
                                  ('output', 9), ('tokens', 3), ('end', 0)])

    def test_llamacpp_native_tool_call_becomes_the_action_line(self):
        common = agentic.common
        def call(args):
            return {'choices': [{'delta': {'tool_calls': [{'index': 0, 'function': {'arguments': args}}]}}]}
        chunks = [{'choices': [{'delta': {'content': 'Let me read.'}}]},
                  call('{"text": "READ h'), call('.cpp:3"}'),
                  {'choices': [{'delta': {}, 'finish_reason': 'tool_calls'}]}]
        with patch.object(common.urllib.request, 'urlopen') as opened, \
                patch.object(common, 'backend_for_model', return_value='http://backend.invalid'):
            opened.return_value.__enter__.return_value = [
                ('data: ' + json.dumps(c) + '\n').encode() for c in chunks]
            answer = common._chat_llamacpp([], model='fixture', tools=server.EXPLOIT_ACTION_TOOLS)
        self.assertEqual(answer, 'READ h.cpp:3\nLet me read.')
        self.assertFalse(common.last_reply_truncated())
        self.assertEqual(json.loads(opened.call_args.args[0].data)['tools'], server.EXPLOIT_ACTION_TOOLS)
        self.assertEqual(server.parse_action(answer), ('READ', 'h.cpp:3', None))

    def test_prewarm_reports_completions(self):
        seen = []
        with patch.object(codecheck, '_var_types', return_value={}):
            codecheck.prewarm_types([self.fi], workers=1,
                                   progress=lambda done, total: seen.append((done, total)))
        self.assertEqual(seen, [(0, 1), (1, 1)])

    def test_ast_decoder_advances_on_the_same_text_object(self):
        roots = [{'mangledName': 'unrelated%d' % i} for i in range(25)]
        roots.append({'mangledName': 'wanted', 'inner': [
            {'kind': 'ParmVarDecl', 'name': 'size', 'type': {'qualType': 'unsigned int'}}]})
        text = '\n \t'.join(json.dumps(node) for node in roots) + '\n'
        decoder = json.JSONDecoder()
        positions = []

        def decode(source, offset=0):
            self.assertIs(source, text)  # copying suffixes caused the quadratic stall
            positions.append(offset)
            return decoder.raw_decode(source, offset)

        with patch.object(codecheck.json, 'JSONDecoder') as factory:
            factory.return_value.raw_decode.side_effect = decode
            self.assertEqual(codecheck._types_from_ast(text, 'wanted'), {'size': 'unsigned int'})
        self.assertEqual(len(positions), len(roots))
        self.assertEqual(positions, sorted(set(positions)))
        factory.assert_called_once()

    def test_ast_cancellation_stops_before_the_next_root(self):
        text = json.dumps({'mangledName': 'unrelated'}) + '\n' + json.dumps({'mangledName': 'wanted'})
        decoder = json.JSONDecoder()

        def decode(source, offset=0):
            result = decoder.raw_decode(source, offset)
            agentic.common.request_cancel()
            return result

        with patch.object(codecheck.json, 'JSONDecoder') as factory:
            factory.return_value.raw_decode.side_effect = decode
            with self.assertRaises(agentic.common.CancelledError):
                codecheck._types_from_ast(text, 'wanted')
        factory.return_value.raw_decode.assert_called_once()

    def test_cancelled_type_job_never_launches_clang_or_reads_cache(self):
        agentic.common.request_cancel()
        with patch.object(codecheck.subprocess, 'run') as run, patch('builtins.open') as opened:
            with self.assertRaises(agentic.common.CancelledError):
                codecheck._var_types(self.fi)
        run.assert_not_called()
        opened.assert_not_called()

    def test_ast_depth_first_order_preserves_first_declaration(self):
        node = {'mangledName': 'wanted', 'inner': [
            {'kind': 'CompoundStmt', 'inner': [
                {'kind': 'VarDecl', 'name': 'n', 'type': {'qualType': 'int'}}]},
            {'kind': 'VarDecl', 'name': 'n', 'type': {'qualType': 'long'}},
            {'kind': 'VarDecl', 'name': 'other', 'type': {'qualType': 'char'}}]}
        self.assertEqual(codecheck._types_from_ast(json.dumps(node), 'wanted'),
                         {'n': 'int', 'other': 'char'})

    def test_interrupt_cancels_pending_type_futures_before_pool_shutdown(self):
        with patch('concurrent.futures.ThreadPoolExecutor') as executor, \
                patch('concurrent.futures.as_completed', side_effect=KeyboardInterrupt):
            pool = executor.return_value.__enter__.return_value
            with self.assertRaises(KeyboardInterrupt):
                codecheck.prewarm_types([self.fi], workers=1)
        pool.submit.return_value.cancel.assert_called_once()
        self.assertTrue(agentic.common._CANCEL_REQUESTED)

    def test_socket_timeout_is_configurable(self):
        with patch.dict(os.environ, {'CC_OLLAMA_TURN_TIMEOUT': '120'}):
            self.assertEqual(agentic.common._ollama_turn_timeout(), 120)

    def test_security_keeps_short_unsafe_and_guarded_bodies(self):
        for body in ("void handler() { memcpy(dst, src, clientLength); }",
                     "bool handler() { return owner == session.player; }",
                     "void handler() { cash -= clientQuantity * price; }"):
            with self.subTest(body=body), patch.object(
                    codetree, "source_body", return_value=(body, 1)):
                self.assertEqual(codecheck.audit_targets(
                    [self.fi], out=io.StringIO(), security=True), [self.fi])
                self.assertEqual(codecheck.audit_targets([self.fi], out=io.StringIO()), [])

    def test_security_keeps_user_destructor(self):
        self.fi.demangled = "Client::~Client"
        with patch.object(codetree, "source_body", return_value=(
                "Client::~Client() { delete pending; }", 1)):
            self.assertFalse(codecheck.is_trivial(self.fi, security=True))
            self.assertTrue(codecheck.is_trivial(self.fi))

    def test_compiler_generated_type_is_still_skipped(self):
        with patch.object(codetree, "source_body", return_value=(
                "struct Packet { int size; };", 1)):
            self.assertTrue(codecheck.is_trivial(self.fi, security=True))

    def test_read_can_reach_tail_and_returns_real_line_numbers(self):
        source = "".join("code_%d;\n" % n for n in range(1, 251))
        with patch.object(agentic, "REPO_ROOT", "/project"), \
                patch("builtins.open", mock_open(read_data=source)):
            self.assertEqual(agentic._tool_read("handler.cpp:220:222"),
                             "220: code_220;\n221: code_221;\n222: code_222;\n")

    def test_read_truncation_has_working_continuation(self):
        source = "".join("statement_%d;\n" % n for n in range(1, 2001))
        with patch.object(agentic, "REPO_ROOT", "/project"), \
                patch("builtins.open", mock_open(read_data=source)):
            first = agentic._tool_read("handler.cpp")
            self.assertLessEqual(len(first), agentic._TOOL_RESULT_CAP)
            self.assertIn("[truncated; continue with READ handler.cpp:", first)
            request = first.split("continue with READ ", 1)[1].split("]", 1)[0]
            next_line = int(request.split(":")[1])
            self.assertIn("%d: statement_%d;" % (next_line, next_line),
                          agentic._tool_read(request))

    def test_invalid_read_is_explicit_and_does_not_open_file(self):
        with patch.object(agentic, "REPO_ROOT", "/project"), \
                patch("builtins.open") as opened:
            for path in ("handler.cpp:0", "handler.cpp:9:2", "../outside.cpp"):
                with self.subTest(path=path):
                    self.assertRegex(agentic._tool_read(path), "error|refused")
            opened.assert_not_called()

    def test_grep_failure_is_not_no_matches(self):
        with patch("subprocess.run") as run:
            run.return_value.returncode = 2
            run.return_value.stderr = "source directory unavailable"
            result = agentic._tool_grep("[index]")
            self.assertIn("grep error", result)
            self.assertIn("source directory unavailable", result)
            command = run.call_args.args[0]
            self.assertIn("-F", command)
            self.assertEqual(command[command.index("--") + 1], "[index]")

    def test_clean_verdict_must_be_explicit(self):
        self.assertEqual(agentic._review_result("NO ISSUES."), "NO ISSUES")
        for answer in ("", "DONE", "INCONCLUSIVE: caller missing", "Probably safe"):
            with self.subTest(answer=answer):
                self.assertIsNone(agentic._review_result(answer))

    def test_native_tool_call_markup_is_unwrapped(self):
        # Reply shapes recorded from a qwen3-coder-class llama.cpp audit run.
        code = "```c\nint main(){return 0;}\n```"
        cases = (
            ("Let me read.\n<tool_call>\n<function=READ>\n<parameter=path>\nh.cpp\n"
             "</parameter>\n</function>\n</tool_call>", ("READ", "h.cpp", None)),
            ('<tool_call>\nfunction>\n<invoke name="READ">\n<parameter=arg_text>/x/h.cpp\n'
             "</parameter>\n</invoke>", ("READ", "/x/h.cpp", None)),
            ("<tool_call>\nREAD\nh.cpp\n</parameter>\n\nNow the caller", ("READ", "h.cpp", None)),
            ("<tool_call>tool_request>GREP heal(", ("GREP", "heal(", None)),
            ("x\n<tool_call>=READ>h.cpp</", ("READ", "h.cpp", None)),
            ('<tool_call>\n{"name": "read", "arguments": {"path": "h.cpp:3"}}\n</tool_call>',
             ("READ", "h.cpp:3", None)),
            ("Writing.\n<tool_call>\n<function=WRITE>\n<parameter=path>\ne.c\n</parameter>\n"
             "<parameter=content>\n" + code + "\n</parameter>\n</function>\n</tool_call>",
             ("WRITE", "e.c", "int main(){return 0;}\n")),
            (code + "\n<tool_call><function=WRITE><parameter=path>e.c</parameter></function>",
             ("WRITE", "e.c", "int main(){return 0;}\n")),
            ("VERDICT FALSEPOSITIVE size>=10 at P.cpp:152", ("VERDICT", "FALSEPOSITIVE", "size>=10 at P.cpp:152")),
            ("Reading.\n<tool_call>\n</function>\n</tool_call>", None),
        )
        for answer, expected in cases:
            with self.subTest(answer=answer):
                self.assertEqual(server.parse_action(answer), expected)
        self.assertEqual(agentic._parse_tool(cases[0][0]), ("READ", "h.cpp"))
        self.assertIsNone(agentic._parse_tool(FINDING))

    def test_action_after_prose_is_parsed(self):
        # Reply shapes recorded from stuck qwen3.8-flash llama.cpp exploit sessions.
        code = "```c\nint main(){return 0;}\n```"
        cases = (
            ("The buffer is strlen(h)+strlen(s)+2 bytes, it fits.\n\nVERDICT FALSEPOSITIVE exact fit at m.cpp:1034",
             ("VERDICT", "FALSEPOSITIVE", "exact fit at m.cpp:1034")),
            ("Let me read the caller.\nREAD server/base/Client.cpp", ("READ", "server/base/Client.cpp", None)),
            ("Writing it.\nWRITE e.c\n" + code, ("WRITE", "e.c", "int main(){return 0;}\n")),
            ("Quoted plan:\n```\nREAD x.cpp\n```\ndone", None),
            ("Some text\nGDB would show the state\nMODE valgrind is better", None),
            ("Let me start by reading the tryCapture function in CommonFightEngineWild.cpp", None),
        )
        for answer, expected in cases:
            with self.subTest(answer=answer):
                self.assertEqual(server.parse_action(answer), expected)
        self.assertIn("VERDICT FALSEPOSITIVE", server.NO_ACTION_NUDGE)

    def test_exploit_read_continues_past_the_cap(self):
        with tempfile.TemporaryDirectory() as root:
            with open(os.path.join(root, "big.cpp"), "w") as source:
                source.write("".join("line%d\n" % i for i in range(1, 101)))
            with patch.object(server, "REPO_ROOT", root), patch.object(server, "TOOL_READ_BYTES", 60):
                first = server.tool_read("big.cpp").splitlines()
                self.assertEqual(first[1], "1\tline1")
                last_shown = int(first[-2].split("\t")[0])
                follow = first[-1].split("continue with READ ", 1)[1].rstrip("]")
                self.assertEqual(server.tool_read(follow).splitlines()[1], "%d\tline%d" % (last_shown + 1, last_shown + 1))
                self.assertEqual(server.tool_read("big.cpp:50:50").splitlines()[1:], ["50\tline50"])
                self.assertIn("has 100 lines", server.tool_read("big.cpp:500"))

    def test_exploit_write_takes_code_from_the_next_reply(self):
        class FakeLive:
            def __init__(self, outdir, mode=None):
                self.alive, self.crashed, self.hung = True, False, False
                self.mode, self.network, self.crash_info = "gdb", None, ""
            def start(self):
                return "listening"
            def stop(self):
                pass
            def poll_crash(self, hard=4):
                return ""
        replies = ["Writing it.\n<tool_call>\n<function=WRITE>\nexploit.c\n</parameter>\n</function>\n</tool_call>",
                   "```c\nint main(){return 0;}\n```",
                   "VERDICT FALSEPOSITIVE length checked at P.cpp:152 before the copy, cannot overflow",
                   "VERDICT FALSEPOSITIVE size>=10 is enforced at P.cpp:152 so the copy is bounded"]
        with tempfile.TemporaryDirectory() as root, \
                patch.object(server, "OUTPUT_ROOT", root), \
                patch.object(server, "LiveServer", FakeLive), \
                patch.object(server, "exploit_reach_context", return_value=""), \
                patch.object(server, "_exploit_chat", side_effect=replies), \
                contextlib.redirect_stderr(io.StringIO()):
            verdict, _, outdir = server.exploit_one("server/cli/main-unix.cpp", FINDING, 1, 600, 500)
            with open(os.path.join(outdir, "exploit.c")) as written:
                self.assertEqual(written.read(), "int main(){return 0;}\n")
        self.assertEqual(verdict, server.VERDICT_REFUTED)

    def test_output_template_is_not_a_finding(self):
        for answer in ('Use SEVERITY(HIGH) | handler.cpp:3 | evidence',
                       'SEVERITY(HIGH)', 'SEVERITY(HIGH) | file:line | evidence'):
            self.assertIsNone(agentic._review_result(answer))

    def test_explicit_clean_verdict_with_explanation_needs_no_retry(self):
        for answer in ("NO ISSUES — the function has an empty body.",
                       "The caller checks the length before copying.\n\nNO ISSUES",
                       "NO ISSUES [done]"):
            with self.subTest(answer=answer), self.review_mocks([answer]) as (_, saved, chat):
                self.assertEqual(self.review(), "NO ISSUES")
                self.assertEqual(chat.call_count, 1)
                self.assertEqual(saved.call_args.args[1], "NO ISSUES")

    def test_clean_marker_does_not_hide_uncertainty_or_cutoff(self):
        for answer in ("If safe, reply NO ISSUES", "NO ISSUES found yet",
                       "NO ISSUES\nINCONCLUSIVE: caller missing",
                       "INCONCLUSIVE: caller missing\nNO ISSUES",
                       "NO ISSUES — empty body\n... [reply truncated at 8000 chars]",
                       "NO ISSUES\nStill investigating the caller."):
            with self.subTest(answer=answer):
                self.assertIsNone(agentic._review_result(answer))

    def test_terminal_clean_marker_never_hides_a_finding(self):
        self.assertEqual(agentic._review_result(FINDING + "\nNO ISSUES"), FINDING)

    def test_truncated_clean_reply_is_not_accepted(self):
        with patch.object(agentic.common,'chat_with',return_value='NO ISSUES'), \
                patch.object(agentic.common,'last_reply_truncated',return_value=True):
            with self.assertRaisesRegex(agentic.ReviewIncomplete,'truncated'):
                agentic._chat(None,[],time.time()+60)

    def test_llamacpp_stream_requires_normal_completion(self):
        common=agentic.common
        for reason in ('stop','length',None):
            chunks=[{'choices':[{'delta':{'content':'NO ISSUES'},'finish_reason':None}]}]
            if reason:
                chunks.append({'choices':[{'delta':{},'finish_reason':reason}]})
            stream=[('data: '+json.dumps(c)+'\n').encode() for c in chunks]
            with self.subTest(reason=reason), \
                    patch.object(common.urllib.request,'urlopen') as opened, \
                    patch.object(common,'backend_for_model',return_value='http://backend.invalid'), \
                    contextlib.redirect_stderr(io.StringIO()):
                opened.return_value.__enter__.return_value=stream
                self.assertEqual(common._chat_llamacpp([],model='fixture'),'NO ISSUES')
                self.assertEqual(common.last_reply_truncated(),reason!='stop')

    def test_exhausted_review_budget_is_not_clean(self):
        with patch.object(agentic,'FUNC_SECS',-1), patch.object(agentic,'_chat') as chat:
            with self.assertRaisesRegex(agentic.ReviewIncomplete,'budget exhausted'):
                agentic.audit_function(None,self.fi,[None])
            chat.assert_not_called()

    def test_oversized_prompt_is_not_sent(self):
        with patch.object(agentic,'_convo_char_budget',return_value=10), \
                patch.object(agentic,'_chat') as chat:
            with self.assertRaisesRegex(agentic.ReviewIncomplete,'context budget'):
                agentic._agentic_review_run(None,[('solo','x'*100)],self.fi,
                                            'review',time.time()+60)
            chat.assert_not_called()

    def test_verdict_cache_depends_on_source_snapshot(self):
        with patch.object(codetree,'_SOURCE_STAMP','old source'):
            first=codecheck._verdict_path(['same visible function'])
        with patch.object(codetree,'_SOURCE_STAMP','changed guard'):
            second=codecheck._verdict_path(['same visible function'])
        self.assertNotEqual(first,second)

    def test_variable_types_belong_to_exact_overload(self):
        self.fi.name='_Z7handleri'
        self.fi.demangled='handler(int)'
        nodes=[{'mangledName':'_Z7handlerPc','inner':[
                    {'kind':'ParmVarDecl','name':'size','type':{'qualType':'char *'}}]},
               {'mangledName':self.fi.name,'inner':[
                    {'kind':'ParmVarDecl','name':'size','type':{'qualType':'int'}}]}]
        opened=mock_open()
        opened.side_effect=[FileNotFoundError(),opened.return_value]
        with patch('builtins.open',opened), \
                patch.object(codetree.os.path,'getmtime',return_value=1), \
                patch.object(codetree,'flags_for',return_value=''), \
                patch.object(codecheck.os,'makedirs'), \
                patch.object(codecheck.subprocess,'run') as run:
            run.return_value.returncode=0
            run.return_value.stdout='\n'.join(json.dumps(n) for n in nodes)
            self.assertEqual(codecheck._var_types(self.fi),{'size':'int'})
            self.assertIn('-ast-dump=json',run.call_args.args[0])

    def test_clean_phrase_does_not_erase_another_finding(self):
        text = "NO ISSUES\n" + FINDING
        self.assertEqual(agentic._review_result(text), FINDING)
        self.assertEqual(agentic._finding_lines(text), [FINDING])

    def test_missing_evidence_fields_does_not_discard_candidate(self):
        self.assertEqual(agentic._review_result(FINDING), FINDING)

    def test_local_model_bare_severity_preserves_candidate(self):
        self.assertEqual(agentic._review_result(
            "high | handler.cpp:6 | client length exceeds destination capacity"),
            "SEVERITY(HIGH) | handler.cpp:6 | client length exceeds destination capacity")

    @contextlib.contextmanager
    def review_mocks(self, replies):
        with patch.object(codetree, "source_body", return_value=("void handler() {}", 1)), \
                patch.object(codecheck, "build_views", return_value=[("solo", "source")]), \
                patch.object(codecheck, "verdict_get", return_value=None) as cached, \
                patch.object(codecheck, "verdict_put") as saved, \
                patch.object(agentic, "_chat", side_effect=replies) as chat, \
                patch.object(agentic, "_convo_char_budget", return_value=100000), \
                patch.object(agentic, "ROUNDS", 1):
            yield cached, saved, chat

    def review(self):
        return agentic._agentic_review(None, None, self.fi, "review", time.time() + 60)

    def test_missing_source_is_incomplete_without_model_or_cached_clean(self):
        with self.review_mocks(["NO ISSUES"]) as (cached, saved, chat), \
                patch.object(codetree, "source_body", return_value=("", 0)):
            with self.assertRaisesRegex(agentic.ReviewIncomplete, "source body unavailable"):
                self.review()
            cached.assert_not_called()
            saved.assert_not_called()
            chat.assert_not_called()

    def test_review_view_contains_cpp_source(self):
        source = "void handler() { memcpy(dst, src, clientLength); }\n"
        with patch("builtins.open", mock_open(read_data=source)), \
                patch.object(codecheck, "headers_for", return_value=[]), \
                patch.object(codetree.TreeRender, "caller_tree", return_value="callers"), \
                patch.object(codecheck, "_var_types", return_value={}), \
                patch.object(codecheck, "tidy_for_function", return_value=[]), \
                patch.object(codecheck, "callee_branches", return_value=[]):
            views = list(codecheck.build_views(None, self.fi))
        self.assertEqual(len(views), 1)
        self.assertIn(source, views[0][1])
        self.assertNotIn("body not found", views[0][1])

    def test_transport_failure_is_incomplete_and_uncached(self):
        with self.review_mocks([TimeoutError("local model timed out")]) as (_, saved, _):
            with self.assertRaisesRegex(agentic.ReviewIncomplete, "timed out"):
                self.review()
            saved.assert_not_called()

    def test_empty_done_is_incomplete_and_uncached(self):
        with self.review_mocks(["DONE"]) as (_, saved, _):
            with self.assertRaises(agentic.ReviewIncomplete):
                self.review()
            saved.assert_not_called()

    def test_unformatted_answer_is_preserved_as_incomplete(self):
        with self.review_mocks(["Need the ownership guard from caller.cpp:90"]) as (_, saved, _):
            with self.assertRaisesRegex(agentic.ReviewIncomplete, "caller.cpp:90"):
                self.review()
            saved.assert_not_called()

    def test_format_repair_reuses_evidence_and_preserves_findings(self):
        prose = "The client length can overflow destination at handler.cpp:3."
        with self.review_mocks([prose, FINDING]) as (_, saved, chat), \
                patch.object(agentic, "ROUNDS", 4):
            self.assertEqual(self.review(), FINDING)
            self.assertEqual(chat.call_count, 2)
            messages = chat.call_args.args[1]
            self.assertIn({"role": "assistant", "content": prose}, messages)
            self.assertEqual(saved.call_args.args[1], FINDING)

    def test_format_repair_can_finish_clean_review(self):
        with self.review_mocks(["The caller checks ownership. Verdict: NO ISSUES",
                                "NO ISSUES"]) as (_, saved, chat), \
                patch.object(agentic, "ROUNDS", 4):
            self.assertEqual(self.review(), "NO ISSUES")
            self.assertEqual(chat.call_count, 2)
            saved.assert_called_once()

    def test_format_repair_is_attempted_only_once(self):
        with self.review_mocks(["Probably safe", "Still probably safe"]) as (_, saved, chat), \
                patch.object(agentic, "ROUNDS", 4):
            with self.assertRaises(agentic.ReviewIncomplete):
                self.review()
            self.assertEqual(chat.call_count, 2)
            saved.assert_not_called()

    def test_missing_evidence_is_not_treated_as_format_error(self):
        with self.review_mocks(["INCONCLUSIVE: caller unavailable"]) as (_, saved, chat), \
                patch.object(agentic, "ROUNDS", 4):
            with self.assertRaisesRegex(agentic.ReviewIncomplete, "caller unavailable"):
                self.review()
            self.assertEqual(chat.call_count, 1)
            saved.assert_not_called()

    def test_budget_final_turn_preserves_candidate(self):
        with self.review_mocks(["BRANCH", FINDING]) as (_, saved, chat):
            self.assertEqual(self.review(), FINDING)
            self.assertEqual(chat.call_count, 2)
            self.assertEqual(saved.call_args.args[1], FINDING)

    def test_budget_final_tool_request_is_incomplete(self):
        with self.review_mocks(["BRANCH", "READ caller.cpp:90"]) as (_, saved, _):
            with self.assertRaises(agentic.ReviewIncomplete):
                self.review()
            saved.assert_not_called()

    def test_next_branch_is_not_hidden_by_repeated_base_context(self):
        base = "header and reviewed body\n" * 500
        branch = "\n=== ONE THING IT CALLS (branch 2/2): validate ===\nownerGuard();\n"
        with self.review_mocks(["BRANCH", "NO ISSUES"]) as (_, _, chat), \
                patch.object(codecheck, "build_views", return_value=[
                    ("branch:first", base), ("branch:validate", base + branch)]):
            self.assertEqual(self.review(), "NO ISSUES")
            results = [m["content"] for m in chat.call_args.args[1]
                       if m["content"].startswith("TOOL RESULT:")]
            self.assertEqual(len(results), 1)
            self.assertIn("ownerGuard();", results[0])
            self.assertNotIn("header and reviewed body", results[0])

    def test_fixed_instructions_are_a_shared_prefix_without_losing_source(self):
        seen = []

        def chat(spec, messages, deadline):
            seen.append([dict(message) for message in messages])
            return "NO ISSUES"

        with patch.object(agentic, "_chat", side_effect=chat), \
                patch.object(agentic, "_convo_char_budget", return_value=100000):
            for body in ("first function and its callers", "second function and its callers"):
                agentic._agentic_review_run(None, [("solo", body)], self.fi,
                                            "security instructions", time.time() + 60)
                self.assertEqual(seen[-1][0]["role"], "system")
                self.assertIn(agentic._TOOL_HELP, seen[-1][0]["content"])
                self.assertEqual(seen[-1][1], {"role": "user", "content": body})
                self.assertEqual("".join(m["content"] for m in seen[-1]).count(
                    agentic._TOOL_HELP), 1)
        self.assertEqual(seen[0][0], seen[1][0])

    def test_default_model_change_invalidates_verdict_cache(self):
        with self.review_mocks(["NO ISSUES", "NO ISSUES"]) as (cached, _, _), \
                patch.object(agentic.common, "USE_CLAUDE", False):
            with patch.object(agentic.common, "MODEL_NAME", "first-model"):
                self.review()
            with patch.object(agentic.common, "MODEL_NAME", "second-model"):
                self.review()
            self.assertNotEqual(cached.call_args_list[0].args[0],
                                cached.call_args_list[1].args[0])

    def test_backend_change_invalidates_same_model_alias(self):
        with self.review_mocks(['NO ISSUES','NO ISSUES']) as (cached,_,_), \
                patch.object(agentic.common,'USE_CLAUDE',False), \
                patch.object(agentic.common,'MODEL_NAME','same-alias'), \
                patch.object(agentic.common,'backend_for_model',side_effect=[
                    'http://first.invalid','http://second.invalid']):
            self.review()
            self.review()
        self.assertNotEqual(cached.call_args_list[0].args[0],cached.call_args_list[1].args[0])

    def test_llamacpp_uses_one_review_worker_by_default(self):
        with patch.object(server.common, "USE_CLAUDE", False), \
                patch.object(server.common, "_ollama_api_kind", return_value="llamacpp"):
            for value in ("", "0", "-1", "invalid"):
                with self.subTest(value=value), patch.dict(
                        os.environ, {"CC_CODECHECK_WORKERS": value}):
                    self.assertEqual(server.codecheck_workers(), 1)

    def test_worker_override_is_preserved(self):
        with patch.dict(os.environ, {"CC_CODECHECK_WORKERS": "3"}):
            self.assertEqual(server.codecheck_workers(), 3)

    def test_other_backends_keep_worker_default(self):
        with patch.dict(os.environ, {"CC_CODECHECK_WORKERS": "0"}), \
                patch.object(server.common, "_ollama_api_kind", return_value="ollama"), \
                patch.object(server.os, "cpu_count", return_value=8):
            self.assertEqual(server.codecheck_workers(), 8)

    def check_server_scan(self, replies, expected_code, completed, proves=False, index_errors=()):
        funcs = [self.fi] * len(replies)
        opened = mock_open()
        output = io.StringIO()
        with contextlib.ExitStack() as stack:
            for owner, name, value in (
                    (agentic, "resolve_llms", [None]),
                    (codecheck, "build_index", SimpleNamespace(errors=index_errors)),
                    (codecheck, "leaves_first", funcs),
                    (codecheck, "prewarm_types", None),
                    (codecheck, "prewarm_tidy", None),
                    (codecheck, "file_sweep", {}),
                    (codetree, "set_cdb_only", None),
                    (codetree, "source_body", ("void handler() {}", 1)),
                    (server, "codecheck_workers", 1)):
                stack.enter_context(patch.object(owner, name, return_value=value))
            stack.enter_context(patch.object(codecheck, "TIDY_CHECKS", "test"))
            stack.enter_context(patch.object(agentic, "audit_function", side_effect=replies))
            proof = stack.enter_context(patch.object(server, "run_exploit", return_value=0))
            stack.enter_context(patch.object(server.os, "makedirs"))
            stack.enter_context(patch("builtins.open", opened))
            stack.enter_context(patch.dict(os.environ, {"CC_CODECHECK_LIMIT": "0"}))
            stack.enter_context(contextlib.redirect_stdout(output))
            stack.enter_context(contextlib.redirect_stderr(output))
            self.assertEqual(server.run_codecheck(), expected_code)
        # The coverage JSON is written before any findings output.
        writes = "".join(call.args[0] for call in opened().write.call_args_list)
        report, _ = json.JSONDecoder().raw_decode(writes)
        self.assertEqual(report["selected"], len(replies))
        self.assertEqual(report["completed"], completed)
        self.assertEqual(len(report["incomplete"]), len(replies) - completed)
        self.assertEqual(proof.called, proves)
        if expected_code:
            self.assertNotIn("no security findings", output.getvalue())

    def test_failed_scan_cannot_report_clean_success(self):
        self.check_server_scan([agentic.ReviewIncomplete("missing caller")], 1, 0)

    def test_review_interrupt_cancels_queued_jobs_before_join(self):
        with patch.object(server.concurrent.futures, 'ThreadPoolExecutor') as executor:
            pool = executor.return_value.__enter__.return_value
            pool.submit.return_value.result.side_effect = KeyboardInterrupt
            with self.assertRaises(KeyboardInterrupt):
                self.check_server_scan([[]], 0, 1)
            pool.shutdown.assert_called_once_with(wait=False, cancel_futures=True)
            self.assertTrue(agentic.common._CANCEL_REQUESTED)

    def test_completed_clean_scan_succeeds_without_proof(self):
        self.check_server_scan([[]], 0, 1)

    def test_empty_index_cannot_report_clean_success(self):
        self.check_server_scan([],1,0)

    def test_failed_index_cannot_report_clean_success(self):
        self.check_server_scan([[]],1,1,index_errors=[('/project/missed.cpp','compile failed')])

    def test_incomplete_scan_still_validates_other_candidates(self):
        self.check_server_scan([[FINDING], agentic.ReviewIncomplete("timeout")],
                               1, 1, proves=True)


class ResourceAndProofRegressionTests(unittest.TestCase):
    """Reproducible CONFIRMED proofs (item 4) and full-sandbox resource limits
    (item 5): what the model uses to prove a crash must not mutate it, harness
    EPERM noise is not containment, output is capped WHILE being received, and
    the whole process tree gets memory/pid limits via a cgroup."""

    def test_gdb_cannot_mutate_the_state_being_proven(self):
        # `print counter++` WRITES memory: the replay afterwards would prove
        # nothing, so any increment/decrement form is refused outright
        for cmd in ("print counter++", "print --refcount", "print x+=1"):
            self.assertFalse(server.gdb_cmd_ok(cmd), cmd)
        self.assertTrue(server.gdb_cmd_ok("print counter"))
        self.assertTrue(server.gdb_cmd_ok("x/8gx &counter"))

    def test_generic_eperm_text_is_not_containment(self):
        # harness-side gdb/ptrace EPERM says nothing about the SERVER
        for text in ("ptrace: Operation not permitted\n(gdb) ",
                     "attach: Operation not permitted"):
            self.assertFalse(any(rx.search(text) for rx in server.CONTAINMENT_BREACH_RES),
                             text)

    def test_path_qualified_eperm_is_containment(self):
        # the kernel prints the path of what was denied: that is the server
        text = "open /etc/shadow: Operation not permitted"
        self.assertTrue(any(rx.search(text) for rx in server.CONTAINMENT_BREACH_RES))

    def test_run_capped_bounds_output_while_receiving(self):
        rc, text, timed_out = server._run_capped(
            [sys.executable, "-c", "import sys; sys.stdout.write('A' * 200000)"],
            cap=1024, timeout=30)
        self.assertEqual(rc, 0)
        self.assertFalse(timed_out)
        self.assertIn("dropped", text)
        # head + tail are kept (min 4096 each) + the notification: bounded
        self.assertLessEqual(len(text), 2 * max(1024 // 2, 4096) + 200)

    def test_run_capped_keeps_head_and_tail_of_a_flood(self):
        rc, text, _ = server._run_capped(
            [sys.executable, "-c",
             "import sys\n"
             "sys.stdout.buffer.write(b'H'*3000); sys.stdout.buffer.flush()\n"
             "sys.stdout.buffer.write(b'M'*200000)\n"
             "sys.stdout.buffer.write(b'T'*3000); sys.stdout.buffer.flush()"],
            cap=100, timeout=30)
        self.assertTrue(text.startswith("H" * 3000))
        self.assertTrue(text.endswith("T" * 3000))

    def test_run_capped_kills_a_process_that_ignores_the_timeout(self):
        start = time.monotonic()
        rc, text, timed_out = server._run_capped(
            [sys.executable, "-c",
             "import time,sys\nsys.stdout.write('x'*200)\nsys.stdout.flush()\n"
             "time.sleep(60)"], cap=4096, timeout=2)
        self.assertLess(time.monotonic() - start, 15)
        self.assertTrue(timed_out)
        self.assertIn("x", text)

    def test_run_cgroup_is_created_joined_and_removed(self):
        old = server.CGROUP_BASE
        with tempfile.TemporaryDirectory(prefix="cgroup-test-") as base:
            server.CGROUP_BASE = base
            try:
                path = server._make_run_cgroup("exploit", 268435456, 64)
                self.assertIsNotNone(path)
                self.assertTrue(path.startswith(base))
                with open(os.path.join(path, "memory.max")) as f:
                    self.assertEqual(f.read().strip(), "268435456")
                with open(os.path.join(path, "pids.max")) as f:
                    self.assertEqual(f.read().strip(), "64")
                # the preexec hook moves the child into the cgroup BEFORE exec,
                # so everything it forks afterwards shares the joint caps
                child = subprocess.Popen(
                    ["/bin/sh", "-c", "read -r v < '%s/cgroup.procs'; echo $v" % path],
                    stdout=subprocess.PIPE, text=True,
                    # partial: the join must DEFER to the child, never run here
                    preexec_fn=functools.partial(server._join_run_cgroup, path))
                pid = int(child.stdout.read().strip())
                child.wait()
                self.assertEqual(pid, child.pid)
                # dropping an emptied cgroup removes it, and never raises
                for name in ("memory.max", "pids.max", "cgroup.procs"):
                    os.unlink(os.path.join(path, name))
                server._drop_run_cgroup(path)
                self.assertFalse(os.path.exists(path))
                server._drop_run_cgroup(None)      # fallback: nothing to do
            finally:
                server.CGROUP_BASE = old

    def test_real_cgroup_applies_limits_when_the_controller_is_delegated(self):
        path = server._make_run_cgroup("selftest", 268435456, 64)
        if path is None:
            self.skipTest("no delegated cgroup v2 subtree here")
        try:
            out = subprocess.run(
                [sys.executable, "-c",
                 "print(open('/proc/self/cgroup').read());"
                 "print(open('%s/memory.max').read())" % path],
                capture_output=True, text=True, timeout=30,
                preexec_fn=functools.partial(server._join_run_cgroup, path)).stdout
            self.assertIn("cc-sec-selftest", out)   # kernel sees it inside
            self.assertIn("268435456", out)         # and under the memory cap
        finally:
            server._drop_run_cgroup(path)


def run_tests():
    loader = unittest.defaultTestLoader
    suite = unittest.TestSuite([
        loader.loadTestsFromTestCase(SecurityReviewTests),
        loader.loadTestsFromTestCase(ResourceAndProofRegressionTests),
    ])
    return unittest.TextTestRunner(failfast=True).run(suite).wasSuccessful()


if __name__ == "__main__":
    sys.exit(0 if run_tests() else 1)
