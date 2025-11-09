#include "_settings.h" // For LANGUAGE_EN etc
#include "system_language.h"

#include "system_captive.h"

Texts* TT[LANGUAGE_COUNT]; // Should only need accessing in system_captive.h
Texts* T; // extern can be accessed anywhere that includes system_language.h

void System_Captive::setupLanguages() {
  // See other TO_ADD_LANGUAGE and TO_ADD_LOCALIZABLE_STRING
  TT[Language_EN] = new Texts { F("EN"), F("English"), 
    F("Language"), 
    F("Project"),
    F("Device Name"),
    F("Description"), 
    F("MQTT host name"),
    F("Captive Portal"),
    F("RESTART"),
    F(" selected"), // Intentional leading space
    F("Connect to WiFi"),
    F("WiFi Network"),
    F("Select one..."),
    F("Password"),
    F("SAVE"),
    F("restarting .... plesae wait"),
    F("Gateway"),
    F("Node"),
    F("Unconnected"),
    F("Set WiFi"),
    F("Tare"),
    F("Calibrate")
  };

  T = TT[Language_EN];

  #if defined(LANGUAGE_FR) || defined(LANGUAGE_ALL)
    TT[Language_FR] = new Texts { F("FR"), F("Francais"), 
      F("Langue"),
      F("Projet"),
      F("Nom de l'appareil"),
      F("Description"),
      F("Nom d'hôte MQTT"),
      F("Portail captif"),
      F("REDÉMARRER"),
      F(" sélectionné"), // Espace de début intentionnel
      F("Connexion au Wi-Fi"),
      F("Réseau Wi-Fi"),
      F("Sélectionnez-en un..."),
      F("Mot de passe"),
      F("ENREGISTRER"),
      F("redémarrage en cours... veuillez patienter"),
      F("Passerelle"),
      F("Nœud"),
      F("Non connecté"),
      F("Set WiFi"),
    F("Tare"),
    F("Calibrate") //TODO-TRANSlATE
      };
  #endif //LANGUAGE_FR

  #if defined(LANGUAGE_SP) || defined(LANGUAGE_ALL)
    TT[Language_SP] = new Texts { F("SP"), F("Espanol"),
      F("Idioma"),
      F("Proyecto"),
      F("Nombre del dispositivo"),
      F("Descripción"),
      F("Nombre del host MQTT"),
      F("Portal cautivo"),
      F("REINICIAR"),
      F(" seleccionado"), // Espacio inicial intencional
      F("Conectarse a Wi-Fi"),
      F("Red Wi-Fi"),
      F("Seleccionar una..."),
      F("Contraseña"),
      F("GUARDAR"),
      F("Reiniciando... espere, por favor"),
      F("Puerta de enlace"),
      F("Nodo"),
      F("Desconectado"),
      F("Set WiFi"),
    F("Tare"),
    F("Calibrate") //TODO-TRANSlATE
 };
  #endif //LANGUAGE_SP

  #if defined(LANGUAGE_DE) || defined(LANGUAGE_ALL)
    TT[Language_DE] = new Texts { F("DE"), F("Deutches"), 
      F("Sprache"),
      F("Projekt"),
      F("Gerätename"),
      F("Beschreibung"),
      F("MQTT-Hostname"),
      F("Captive Portal"),
      F("Neustart"),
      F(" Ausgewählt"), // Absichtliches Leerzeichen am Anfang
      F("Mit WLAN verbinden"),
      F("WLAN-Netzwerk"),
      F("Wählen Sie eins aus..."),
      F("Passwort"),
      F("Speichern"),
      F("Neustart .... Bitte warten"),
      F("Gateway"),
      F("Knoten"),
      F("Nicht verbunden"),
      F("Set WiFi"),
    F("Tare"),
    F("Calibrate") //TODO-TRANSlATE
      };
  #endif //LANGUAGE_DE

  #if defined(LANGUAGE_NL) || defined(LANGUAGE_ALL)
    TT[Language_NL] = new Texts { F("NL"), F("Nederlands"), 
      F("Taal"),
      F("Project"),
      F("Apparaatnaam"),
      F("Beschrijving"),
      F("MQTT-hostnaam"),
      F("Captive Portal"),
      F("OPNIEUW STARTEN"),
      F(" geselecteerd"), // Opzettelijke voorloopspatie
      F("Verbinding maken met wifi"),
      F("Wifi-netwerk"),
      F("Selecteer er één..."),
      F("Wachtwoord"),
      F("OPSLAAN"),
      F("opnieuw opstarten.... even geduld a.u.b."),
      F("Gateway"),
      F("Knooppunt"),
      F("Niet verbonden"),
      F("Set WiFi"),
    F("Tare"),
    F("Calibrate") //TODO-TRANSlATE
      };
  #endif //LANGUAGE_NL

  #if defined(LANGUAGE_ID) || defined(LANGUAGE_ALL)
    TT[Language_ID] = new Texts { F("ID"), F("Bahasa Indonesia"), 
      F("Bahasa"),
      F("Proyek"),
      F("Nama Perangkat"),
      F("Deskripsi"),
      F("Nama host MQTT"),
      F("Portal Captive"),
      F("MULAI ULANG"),
      F(" dipilih"), // Spasi awal yang disengaja
      F("Hubungkan ke WiFi"),
      F("Jaringan WiFi"),
      F("Pilih satu..."),
      F("Kata Sandi"),
      F("SIMPAN"),
      F("sedang memulai ulang .... mohon tunggu"),
      F("Gerbang"),
      F("Node"),
      F("Tidak Terhubung"),
      F("Set WiFi"),
    F("Tare"),
    F("Calibrate") //TODO-TRANSlATE
      };
  #endif //LANGUAGE_ID

  #if defined(LANGUAGE_HI) || defined(LANGUAGE_ALL)
    TT[Language_HI] = new Texts { F("HI"), F("Hindi"), 
      F("भाषा"),
      F("प्रोजेक्ट"),
      F("डिवाइस का नाम"),
      F("विवरण"),
      F("MQTT होस्ट नाम"),
      F("कैप्टिव पोर्टल"),
      F("पुनः प्रारंभ करें"),
      F("चयनित"), // जानबूझकर स्पेस दिया गया है
      F("वाई-फ़ाई से कनेक्ट करें"),
      F("वाई-फ़ाई नेटवर्क"),
      F("एक चुनें..."),
      F("पासवर्ड"),
      F("सहेजें"),
      F("पुनः प्रारंभ हो रहा है.... कृपया प्रतीक्षा करें"),
      F("गेटवे"),
      F("नोड"),
      F("असंबद्ध"),
      F("Set WiFi"),
    F("Tare"),
    F("Calibrate") //TODO-TRANSlATE
      };
  #endif //LANGUAGE_HI
}
