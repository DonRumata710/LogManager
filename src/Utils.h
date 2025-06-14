#pragma once


#define QT_SLOT_BEGIN \
    try {

#define QT_SLOT_END \
    } catch (const std::exception& e) { \
        qCritical() << "Exception in slot:" << e.what(); \
    } catch (...) { \
        qCritical() << "Unknown exception in slot"; \
    }
