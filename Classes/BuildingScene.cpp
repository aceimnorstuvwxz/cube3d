// 湍流游戏


#include <cstdio>
#include <vector>

#include "../rapidjson/rapidjson.h"
#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"
#include "../rapidjson/filereadstream.h"

#include <sys/stat.h> //mkdir

#include "BuildingScene.h"
#include "uiconf.h"
#include "EditState.h"
#include "format.h"
#include "Msg.h"
#include "CubeSprite.h"
#include "EditState.h"


USING_NS_CC;

const float MOVE_SCALE = 1.f;
const float UP_DOWN_MAX = 0.99f;
const float ROTATE_SCALE = 0.01f;
const float SCALL_MOVE_SCALE = 0.1f;
const float PI = 3.1415926f;
class StateInfoLayer: public McdLayer
{
public:
    virtual bool init() override {
        assert(McdLayer::init());

        _lbUnitFile = Label::createWithTTF(EditState::s()->getExtraUnitFilePath(), uic::font_zh, 14);
        _lbUnitFile->setPosition(genPos({0.5f, 0.95f}));
        this->addChild(_lbUnitFile);

        schedule(schedule_selector(StateInfoLayer::onUpdateState), 0.33f, kRepeatForever, 0);

        return true;
    }
    CREATE_FUNC(StateInfoLayer);

private:
    Label* _lbUnitFile;

    void onUpdateState(float dt) {
        _lbUnitFile->setString(EditState::s()->getExtraUnitFilePath());
    }

};


bool BuildingScene::init()
{
    assert(TRBaseScene::init());

    this->addChild(StateInfoLayer::create());

    _brushLayer = BrushLayer::create();
    this->addChild(_brushLayer);

    reloadMetaCubes();

    initMenuButtons();

    initSceneLayer();

    scheduleUpdate();

    return true;
}

void BuildingScene::reloadMetaCubes()
{

    //clear old
    EditState::s()->getMetaCubeMap()->clear();

    FILE* fp = fopen(EditState::s()->getCubeMetaFilePath().c_str(), "r"); // TODO if Windows using "rb"

    if (fp == NULL) {
        CCLOG("Metacubes not exists, copy template workspace there.");
        copyTemplateWorkspace();
        // reopen
        fp = fopen(EditState::s()->getCubeMetaFilePath().c_str(), "r");
    }

    assert(fp != NULL);
    
    char readBuffer[65536];
    rjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rjson::Document doc;
    doc.ParseStream(is);
    fclose(fp);

    // extract metacubes
    // defaults
    assert(doc.HasMember("default_shader"));
    auto& defaultShader = doc["default_shader"];
    assert(defaultShader.Size() == 2);
    int shaderIndex = 0;
    for (auto iter = defaultShader.Begin(); iter != defaultShader.End(); iter++, shaderIndex++) {
        if (shaderIndex == 0) {
            EditState::s()->setDefaultVertexShader(iter->GetString());
        } else {
            EditState::s()->setDefaultFragmentShader(iter->GetString());
        }
    }
    assert(doc.HasMember("list"));
    auto& list = doc["list"];
    auto getStr = [](const rjson::Value& obj, const char* key) {
        if (obj.HasMember(key)) {
            assert(obj[key].IsString());
            return std::string(obj[key].GetString());
        }
        return std::string("");
    };
    for (auto iter = list.Begin(); iter != list.End(); iter++) {
        MetaCube metacube;
        //id
        assert(iter->HasMember("id"));
        metacube.id = (*iter)["id"].GetInt();
        //name
        metacube.name = getStr(*iter, "name");
        //comment
        metacube.comment = getStr(*iter, "comment");
        //texture
        metacube.texture = getStr(*iter, "texture");
        //color
        if (iter->HasMember("color")) {
            auto& color = (*iter)["color"];
            assert(color.IsArray() && color.Size() == 4);
            int tmpIndex = 0;
            float colordata[4];
            for (auto i = color.Begin(); i != color.End(); i++, tmpIndex++) {
                float a = i->GetDouble();
                assert(a >= 0.f && a <= 1.f);
                colordata[tmpIndex] = a;
            }
            metacube.color = {colordata[0], colordata[1], colordata[2], colordata[3]};
        }
        //shader
        if (iter->HasMember("shader")) {
            auto& shader = (*iter)["shader"];
            assert(shader.IsArray() && shader.Size() == 2);
            int tmpIndex = 0;
            for (auto i = shader.Begin(); i != shader.End(); i++, tmpIndex++) {
                assert(i->IsString());
                metacube.shader[tmpIndex] = i->GetString();
            }
        }
        //unreal
        if (iter->HasMember("unreal")) {
            auto& unreal = (*iter)["unreal"];
            assert(unreal.IsInt());
            metacube.unreal = unreal.GetInt() != 0;
        }

        metacube.reload();
        (*(EditState::s()->getMetaCubeMap()))[metacube.id] = metacube;
    }

    //extract metacubes from file is done. Now we will display them.
    _brushLayer->reload();
}

void BuildingScene::copyTemplateWorkspace()
{
    auto fileListString = FileUtils::getInstance()->getStringFromFile("MicroCube/filelist.json");
    CCLOG("%s", fileListString.c_str());
    rjson::Document doc;
    doc.Parse(fileListString.c_str());
    std::vector<std::string> dirs;
    std::vector<std::string> files;
    auto& dirlist = doc["directories"];
    for (auto iter = dirlist.Begin(); iter != dirlist.End(); iter++) {
        std::string str = iter->GetString();
        CCLOG("%s", str.c_str());
        dirs.push_back(str);
    }
    auto& filelist = doc["files"];
    for (auto iter = filelist.Begin(); iter != filelist.End(); iter++) {
        std::string str = iter->GetString();
        CCLOG("%s", str.c_str());
        files.push_back(str);
    }
    // create base dir
    mkdir(EditState::s()->getBasePath().c_str(), 0777);
    for (int i = 0; i < dirs.size(); i++) {
        std::string dst = EditState::s()->getBasePath()+dirs[i];
        mkdir(dst.c_str(), 0777);
    }
    for (int i = 0; i < files.size(); i++) {
        auto data = FileUtils::getInstance()->getDataFromFile("MicroCube/" + files[i]);
        std::string dst = EditState::s()->getBasePath()+files[i];
        CCLOG("%s", dst.c_str());
        FILE* fp = fopen((EditState::s()->getBasePath()+files[i]).c_str(), "w");
        assert(fp != NULL);
        fwrite(data.getBytes(), sizeof(unsigned char), data.getSize(), fp);
        fclose(fp);
    }

}

void BuildingScene::initMenuButtons()
{
    addCommonBtn({0.1f,0.9f}, Msg::s()["unreal_bottom"], [this](){ addUnrealWall(UY, 2);
    });
}

void BuildingScene::initSceneLayer()
{
    _sceneLayer = Layer::create();
    this->addChild(_sceneLayer);

    auto size = Director::getInstance()->getVisibleSize();

    _sceneCamera = Camera::createPerspective(60, size.width/size.height, 1, 1000);
    _sceneCamera->setPosition3D({0, 0, 200});
    _sceneCamera->lookAt({0,0,0});
    _sceneCamera->setCameraFlag(CameraFlag::USER2);
    _sceneLayer->addChild(_sceneCamera);
    _sceneLayer->setCameraMask((unsigned short)CameraFlag::USER2);

    // Mouse and keyboard

    // keyboard
    auto _keyboardListener = EventListenerKeyboard::create();
    _keyboardListener->onKeyPressed = [&](EventKeyboard::KeyCode code, Event* event){
        switch (code) {
            case EventKeyboard::KeyCode::KEY_SPACE:
            CCLOG("key space, shot");
            //TODO
            break;

            case EventKeyboard::KeyCode::KEY_W:
            case EventKeyboard::KeyCode::KEY_CAPITAL_W:
            CCLOG("key W down");
            _zoomIn = true;
            break;

            case EventKeyboard::KeyCode::KEY_S:
            case EventKeyboard::KeyCode::KEY_CAPITAL_S:
            CCLOG("key S down");
            _zoomOut = true;
            break;

            case EventKeyboard::KeyCode::KEY_A:
            case EventKeyboard::KeyCode::KEY_CAPITAL_A:
            CCLOG("key A down");
            _left = true;
            break;

            case EventKeyboard::KeyCode::KEY_D:
            case EventKeyboard::KeyCode::KEY_CAPITAL_D:
            CCLOG("key D down");
            _right = true;
            break;

            case EventKeyboard::KeyCode::KEY_Q:
            case EventKeyboard::KeyCode::KEY_CAPITAL_Q:
            CCLOG("key Q down");
            _up = true;
            break;

            case EventKeyboard::KeyCode::KEY_E:
            case EventKeyboard::KeyCode::KEY_CAPITAL_E:
            CCLOG("key E down");
            _down = true;
            break;

            default:
            break;
        }
    };
    _keyboardListener->onKeyReleased = [&](EventKeyboard::KeyCode code, Event* event){
        switch (code) {
            case EventKeyboard::KeyCode::KEY_SPACE:
            CCLOG("key space, shot");
            //TODO
            break;

            case EventKeyboard::KeyCode::KEY_W:
            case EventKeyboard::KeyCode::KEY_CAPITAL_W:
            CCLOG("key W up");
            _zoomIn = false;
            break;

            case EventKeyboard::KeyCode::KEY_S:
            case EventKeyboard::KeyCode::KEY_CAPITAL_S:
            CCLOG("key S up");
            _zoomOut = false;
            break;

            case EventKeyboard::KeyCode::KEY_A:
            case EventKeyboard::KeyCode::KEY_CAPITAL_A:
            CCLOG("key A up");
            _left = false;
            break;

            case EventKeyboard::KeyCode::KEY_D:
            case EventKeyboard::KeyCode::KEY_CAPITAL_D:
            CCLOG("key D up");
            _right = false;
            break;
            case EventKeyboard::KeyCode::KEY_Q:
            case EventKeyboard::KeyCode::KEY_CAPITAL_Q:
            CCLOG("key Q up");
            _up = false;
            break;

            case EventKeyboard::KeyCode::KEY_E:
            case EventKeyboard::KeyCode::KEY_CAPITAL_E:
            CCLOG("key E up");
            _down = false;
            break;

            default:
            break;
        }
        
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(_keyboardListener, this);

    // mouse
    auto _mouseListener = EventListenerMouse::create();

    _mouseListener->onMouseMove = [&](Event *event){
        if (!_isFPS) return;

        EventMouse* e = (EventMouse*)event;
        auto now = e->getLocation();
        Vec2 diff = now - _fpsAnchor;
        _fpsAnchor = now;
        _rotateY -= ROTATE_SCALE*diff.x;
        _rotateX -= ROTATE_SCALE*diff.y;
        if (_rotateX > UP_DOWN_MAX*0.5f*PI) _rotateX = UP_DOWN_MAX*0.5f*PI;
        if (_rotateX < -UP_DOWN_MAX*0.5f*PI) _rotateX = -UP_DOWN_MAX*0.5f*PI;

    };
    _mouseListener->onMouseUp = [&](Event *event){
        EventMouse* e = (EventMouse*)event;
        switch(e->getMouseButton()){
            case MOUSE_BUTTON_LEFT:
            break;

            case MOUSE_BUTTON_RIGHT:
            _isFPS = false;
            break;

            default:
            break;
        }
    };
    _mouseListener->onMouseDown = [&](Event *event){
        EventMouse* e = (EventMouse*)event;
        switch(e->getMouseButton()){
            case MOUSE_BUTTON_LEFT:
            break;

            case MOUSE_BUTTON_RIGHT:
            {
                _isFPS = true;
                _fpsAnchor = e->getLocation();
                auto ro = _sceneCamera->getRotation3D();
                CCLOG("%f,%f,%f", ro.x, ro.y, ro.z);
                break;
            }
            default:
            break;
        }
    };
    _mouseListener->onMouseScroll = [&](Event *event){
        EventMouse* e = (EventMouse*)event;
        CCLOG("%f, %f", e->getScrollX(), e->getScrollY());
        float diff = e->getScrollY();
        Vec3 dir = _sceneCamera->getRotationQuat() * Vec3{0.f, 0.f, -1.f};
        dir.normalize();
        _sceneCamera->setPosition3D(_sceneCamera->getPosition3D() + SCALL_MOVE_SCALE * -diff * dir);
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(_mouseListener, this);

}

cocos2d::Vec3 BuildingScene::rawPos2Real(cocos2d::Vec3 raw)
{
    auto ret = 10.f*raw;
    CCLOG("%f %f %f %f %f %f", raw.x, raw.y, raw.z, ret.x, ret.y,ret.z);
    return ret;
}

void BuildingScene::addCube(CubeSprite* cube)
{
    assert(cube);

    // Add to HashTable
//    _cubeMap[cube->getRawPos()] = cube;
    _cubes.push_back(cube); //crash

    // Add to Scene
    cube->setPosition3D(rawPos2Real(cube->getRawPos()));
    cube->setCameraMask(_sceneLayer->getCameraMask());
    _sceneLayer->addChild(cube);
}

void BuildingScene::removeCube(CubeSprite* cube)
{
    assert(cube);
}


void BuildingScene::addUnrealWall(UnrealType t, int width)
{
    assert(width >= 0);
    for (int i = 0-width/2; i < width/2; i++) {
        for (int j = 0-width/2; j < width/2; j++) {
            Vec3 rawPos = {0,0,0};
            switch (t) {
                case UX:
                rawPos = Vec3{0, 1.0f*i, 1.0f*j};
                break;

                case UY:
                rawPos = Vec3{1.f*i, 0 ,1.f*j};
                break;

                case UZ:
                rawPos = Vec3{1.f*i, 1.f*j, 0};
                break;
            }
            addCube(CubeSprite::create(rawPos, _brushLayer->getSelectedCubeId()));
        }
    }
//    addCube(CubeSprite::create({0,0,0}, 0));
}

void BuildingScene::update(float dt)
{
    Quaternion qua2 = {Vec3(1.f,0.f,0.f), _rotateX};
    Quaternion qua = {Vec3(0.f, 1.f, 0.f), _rotateY};
    qua.multiply(qua2);
    _sceneCamera->setRotationQuat(qua);

    qua.normalize();
    if (_up && !_down) {
        Vec3 dir = qua * Vec3{0.f,1.f,0.f};
        dir.normalize();
        _sceneCamera->setPosition3D(_sceneCamera->getPosition3D() + MOVE_SCALE * dir);
    }
    if (_down && !_up) {
        Vec3 dir = qua * Vec3{0.f,-1.f,0.f};
        dir.normalize();
        _sceneCamera->setPosition3D(_sceneCamera->getPosition3D() + MOVE_SCALE * dir);
    }
    if (_left && !_right) {
        Vec3 dir = qua * Vec3{-1.f,0.f,0.f};
        dir.normalize();
        _sceneCamera->setPosition3D(_sceneCamera->getPosition3D() + MOVE_SCALE * dir);
    }
    if (_right && !_left) {
        Vec3 dir = qua * Vec3{1.f,0.f,0.f};
        dir.normalize();
        _sceneCamera->setPosition3D(_sceneCamera->getPosition3D() + MOVE_SCALE * dir);
    }
    if (_zoomIn && !_zoomOut) {
        Vec3 dir = qua * Vec3{0.f,0.f,-1.f};
        dir.normalize();
        _sceneCamera->setPosition3D(_sceneCamera->getPosition3D() + MOVE_SCALE * dir);
    }
    if (_zoomOut && !_zoomIn) {
        Vec3 dir = qua * Vec3{0.f,0.f,1.f};
        dir.normalize();
        _sceneCamera->setPosition3D(_sceneCamera->getPosition3D() + MOVE_SCALE * dir);
    }
}
