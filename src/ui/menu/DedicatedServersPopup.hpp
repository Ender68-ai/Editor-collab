#pragma once
#include <Geode/Geode.hpp>
#include "../core/BasePopup.hpp"

namespace mpedit {

    struct SavedServer {
        std::string name;
        std::string url;
    };

    class DedicatedServersPopup : public BasePopup {
    protected:
        geode::ScrollLayer* m_scrollLayer = nullptr;
        cocos2d::extension::CCScale9Sprite* m_bg = nullptr;
        cocos2d::CCLabelBMFont* m_statusLabel = nullptr;
        std::function<void(std::string const&)> m_onConnect;

        bool init(std::function<void(std::string const&)> onConnect);

        
        void onAddServer(cocos2d::CCObject*);
        void onDirectConnect(cocos2d::CCObject*);
        


    public:
        static DedicatedServersPopup* create(std::function<void(std::string const&)> onConnect);
        void connectTo(std::string const& url);
        void deleteServer(int index);
        std::vector<SavedServer> getSavedServers();
        void saveServers(std::vector<SavedServer> const& servers);
        void setupList();
    };

}
