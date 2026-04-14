#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

class CollabRoomPopup : public geode::Popup {
private:
    std::string m_roomCode;
    
protected:
    bool init(std::string const& roomCode);
    void onClose(cocos2d::CCObject* sender) override;
    void onHost(cocos2d::CCObject* sender);
    void onJoin(cocos2d::CCObject* sender);
    
public:
    static CollabRoomPopup* create(std::string const& roomCode);
    void setRoomCode(std::string const& roomCode);
};