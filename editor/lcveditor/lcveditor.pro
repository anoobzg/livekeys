TEMPLATE = lib
TARGET   = lcveditor
QT      += core qml quick
CONFIG  += qt

win32:{
    DESTDIR    = $$BUILD_PWD/lib
    DLLDESTDIR = $$DEPLOY_PATH
}else:DESTDIR = $$DEPLOY_PATH

DEFINES += Q_LCVEDITOR_LIB

include($$PWD/src/lcveditor.pri)
include($$PWD/include/lcveditorheaders.pri)
