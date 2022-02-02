contains(DEFINES, __ANDROID_API__) {
VIEWPORT_BASE = $$PWD/mobile
}
!contains(DEFINES, __ANDROID_API__) {
VIEWPORT_BASE = $$PWD/desktop 
}

SOURCES += $$VIEWPORT_BASE/Battle.cpp
