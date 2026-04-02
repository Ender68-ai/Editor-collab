#pragma once
#include <Geode/Geode.hpp>
#include "CollabManager.hpp"
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

class CollabSetupPopup : public Popup {
protected:
    std::tuple<std::string, int> m_data;
    bool init(const std::string& levelName, int levelID);
    void onEnable(CCObject* sender);
    
public:
    static CollabSetupPopup* create(const std::string& levelName, int levelID);
};

class CollabManagementPopup : public Popup {
protected:
    int m_data;
    bool init(int levelID);
    void setupPlayerList();
    void onAddPlayer(CCObject* sender);
    void onBack(CCObject* sender);
    void onTogglePlayer(CCObject* sender);
    
public:
    static CollabManagementPopup* create(int levelID);
};

class TransferConfirmPopup : public Popup {
protected:
    std::tuple<int, int, std::string> m_data;
    bool init(int levelID, int targetAccountID, const std::string& levelName);
    void onConfirm(CCObject* sender);
    void onCancel(CCObject* sender);
    
public:
    static TransferConfirmPopup* create(int levelID, int targetAccountID, const std::string& levelName);
};

class TransferRequestPopup : public Popup {
protected:
    TransferRequest m_data;
    bool init(const TransferRequest& request);
    void onAccept(CCObject* sender);
    void onDecline(CCObject* sender);
    
public:
    TransferRequestPopup() = default;
    static TransferRequestPopup* create(const TransferRequest& request);
};

class CollabSettingsPopup : public Popup {
protected:
    int m_levelID;
    std::string m_levelName;
    bool m_collabEnabled;
    
    bool init(int levelID, const std::string& levelName);
    void onToggleRealTimeCollab(CCObject* sender);
    void onClose(CCObject* sender);
    
public:
    static CollabSettingsPopup* create(int levelID, const std::string& levelName);
};

class FirstTimeSetupPopup : public Popup {
protected:
    bool init();
    void onNext(CCObject* sender);
    void onSkip(CCObject* sender);
    
public:
    static FirstTimeSetupPopup* create();
};
