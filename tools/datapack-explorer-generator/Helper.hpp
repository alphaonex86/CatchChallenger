#ifndef HELPER_HPP
#define HELPER_HPP

#include <string>
#include <vector>

namespace Helper
{
    // Create directory recursively (mkdir -p)
    bool mkpath(const std::string &path);

    // Write content to file (creates parent dirs)
    bool writeFile(const std::string &path, const std::string &content);

    // Read text file fully
    bool readFile(const std::string &path, std::string &out);

    // File existence / directory check
    bool fileExists(const std::string &path);
    bool isDir(const std::string &path);

    // Replace all occurrences
    std::string replaceAll(std::string str, const std::string &from, const std::string &to);

    // Basic text transforms corresponding to function.php helpers
    std::string toLowerCase(const std::string &text);
    std::string cleanText(const std::string &text, size_t maximumStringLength = 64);
    std::string textForUrl(const std::string &text);
    std::string firstLetterUpper(const std::string &text);
    std::string lowerCaseFirstLetterUpper(const std::string &text);

    // HTML escape
    std::string htmlEscape(const std::string &text);

    // Return true if path ends with suffix
    bool endsWith(const std::string &str, const std::string &suffix);

    // List all .tmx files recursively (paths relative to dir)
    std::vector<std::string> getTmxList(const std::string &dir, const std::string &subDir = std::string());
    std::vector<std::string> getXmlList(const std::string &dir, const std::string &subDir = std::string());

    // Join a base folder with sub relative paths safely
    std::string pathJoin(const std::string &a, const std::string &b);

    // Convert integer-ish number to std::string
    std::string toStringInt(long long v);
    std::string toStringUint(unsigned long long v);

    // Template html globally loaded (with ${TITLE}, ${CONTENT}, ${AUTOGEN})
    void setTemplate(const std::string &t);
    const std::string &templateHtml();

    // Paths globals
    void setLocalPath(const std::string &p);
    void setDatapackPath(const std::string &p);
    void setMainDatapackCode(const std::string &code);
    void setSubDatapackCode(const std::string &code);
    const std::string &localPath();
    const std::string &datapackPath();
    const std::string &mainDatapackCode();
    const std::string &subDatapackCode();

    void setMap2PngPath(const std::string &p);
    const std::string &map2pngPath();

    // Track the current page being generated, used to compute relative
    // URLs from links back to other resources.
    void setCurrentPage(const std::string &relativePath);
    const std::string &currentPage();

    // Compute a relative URL (from the current page directory) to the
    // specified absolute-from-output-root target path. If no current page is
    // set, the target is returned unchanged.
    std::string relUrl(const std::string &targetRelativePath);

    // Stateless version: compute a relative URL from the directory of
    // "fromPage" (itself a path relative to the output root, e.g.
    // "items/food/donus.html") to "targetRelativePath" (also relative to
    // the output root, e.g. "types/9.html").
    std::string relUrlFrom(const std::string &fromPage,
                           const std::string &targetRelativePath);

    // Write a full HTML page composed of header + body + footer.
    // The current page is set to relativePath for the duration of this call.
    void writeHtml(const std::string &relativePath, const std::string &title, const std::string &body);

    // Return a relative URL (from current page) to a datapack image at
    // the given absolute filesystem path, by copying the image to
    // <localpath>/datapack/<rel> and producing the corresponding link.
    std::string datapackImageUrl(const std::string &absPath);

    // Strip the datapack prefix from an absolute path, returning the remainder
    // (e.g. "items/foo.png"). If not under datapack, returns path unchanged.
    std::string relativeFromDatapack(const std::string &absPath);

    // Copy a file (used for images from the datapack copied to the output).
    bool copyFile(const std::string &src, const std::string &dst);

    // Ensure a datapack file (relative from datapack root) is copied to the
    // output directory under "datapack/<rel>" and return the path (relative
    // to the output root, not the current page).
    std::string publishDatapackFile(const std::string &datapackRelative);
}

#endif // HELPER_HPP
