#ifndef SYSTEM_LANGUAGE_H
#define SYSTEM_LANGUAGE_H

// See other #TO_ADD_LANGUAGE
enum Language_t {  Language_EN, Language_FR, Language_SP, Language_DE, Language_NL, Language_ID, Language_HI, LANGUAGE_COUNT };

// TO_ADD_LOCALIZABLE_STRING add each new localizable string here
struct Texts {
    const __FlashStringHelper
        *code,
        *LanguageName,
        *Language,
        *Project,
        *DeviceName,
        *Description,
        *MQTThostname,
        *CaptivePortal,
        *RESTART,
        *_selected,
        *ConnectToWiFi,
        *WiFiNetwork,
        *SelectOne,
        *Password,
        *SAVE,
        *RestartingPleaseWait,
        *LoraMesher_Gateway,
        *LoraMesher_Node,
        *LoraMesher_Unconnected,
        *SETWIFI,
        *Tare,
        *Calibrate
        ;
};

extern Texts* T;  // Will be set to current language so strings accessed as e.g. T->DeviceName
extern Texts* TT[LANGUAGE_COUNT]; // Should only need accessing in system_captive.h

#endif