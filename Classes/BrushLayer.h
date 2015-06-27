// 湍流游戏

// 笔刷及笔刷选择，是BuildingScene的一部分。
#ifndef __cube3d__BrushLayer__
#define __cube3d__BrushLayer__

#include "cocos2d.h"
#include <unordered_map>
#include <vector>
#include "CubeSprite.h"
#include "McdLayer.h"

class BrushLayer: public McdLayer
{
public:
    virtual bool init() override;
    CREATE_FUNC(BrushLayer);

    void reload();
    int getSelectedCubeId(){return _selectedMetaCubeId;}

private:

    std::unordered_map<int, CubeSprite*> _cubeMap;
    CubeSprite* _selectedCube = nullptr;
    int _selectedMetaCubeId;
    void setNewSelectedCube(int metaCubeId);
    cocos2d::Camera* _camera;
    bool _isCubesVisible = false;
    void hideOrShowAllMetaCubes();


};

#endif /* defined(__cube3d__BrushLayer__) */
