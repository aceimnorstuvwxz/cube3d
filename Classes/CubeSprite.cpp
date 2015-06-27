// 湍流国际

// 这个类的形式必须使用修改过的cocos2d。

#include "CubeSprite.h"
#include "EditState.h"
#include <vector>
USING_NS_CC;

CubeSprite* CubeSprite::create(const cocos2d::Vec3& pos_raw, int metaCubeId)
{
    auto sprite = new (std::nothrow) CubeSprite();

    if (sprite && sprite->init(pos_raw, metaCubeId)) {
        sprite->autorelease();
        return sprite;
    }

    CC_SAFE_DELETE(sprite);
    return nullptr;
}

bool CubeSprite::init(const cocos2d::Vec3& pos_raw, int metaCubeId)
{

    assert(Node::init());

    _metaCubeId = metaCubeId;
    _blendFunc = BlendFunc::ALPHA_NON_PREMULTIPLIED;
    MetaCube* metaCube = EditState::s()->getMetaCube(metaCubeId);
    // 这个共享机制，还可以根据普通2D的Sprite来修改。
    this->setGLProgramState(metaCube->getProgramState());
    _color = EditState::s()->getMetaCube(metaCubeId)->color;
    _renderMesh = metaCube->getRenderMesh();
    _texture = metaCube->getTexture();
    _posRaw = pos_raw;
    
    return true;
}

void CubeSprite::draw(cocos2d::Renderer* renderer, const cocos2d::Mat4& transform, uint32_t flags)
{
    CC_SAFE_DELETE(_meshCommand);
    _meshCommand = new MeshCommand();
    _meshCommand->genMaterialID(0, getGLProgramState(), _renderMesh->getVertexBuffer(), _renderMesh->getIndexBuffer(), _blendFunc);
    _meshCommand->init(_globalZOrder, 0, getGLProgramState(), _blendFunc, _renderMesh->getVertexBuffer(), _renderMesh->getIndexBuffer(), (GLenum)_renderMesh->getPrimitiveType(), (GLenum)_renderMesh->getIndexFormat(), _renderMesh->getIndexCount(), transform, flags);
    _meshCommand->setCullFaceEnabled(true);
    _meshCommand->setDepthTestEnabled(true);
    _meshCommand->setTransparent(true);

    Vec4 color = _color;
    // hacked。这里用color.a == 0来在fsh中判断是否是texture还是color，当texture时，咱们用color.r来传alpha值。
    if (_texture){
        getGLProgramState()->setUniformTexture("u_texture", _texture);
        color.x = getOpacity() * 1.f / 256.f;
    } else {
        color.w = color.w * (getOpacity() * 1.f / 256.f);
    }
    _meshCommand->setDisplayColor(color);
    renderer->addCommand(_meshCommand);
    Node::draw(renderer, transform, flags);
}