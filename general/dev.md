# Dev choice
* map index is int, fast to search, fast to compare no way to have problem between full and relative path if string, drop pointer problem (no pointer resolution/recreate if restaure from cache like in datapack cache), copy with take care of use after free, less size (16Bits vs 32/64Bits)
