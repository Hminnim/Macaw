#pragma once

#include "UPrimitiveComponent.h"

struct FMatrix;

// 카메라를 향하는 Primitive의 공통 기반 클래스.
// Billboard의 실제 방향 계산은 Shader에서 수행한다.
// 이 클래스는 렌더링 여부와 Billboard 원점만 제공한다.

class UBillboardComponent : public UPrimitiveComponent
{
public:
    UBillboardComponent() = default;
    ~UBillboardComponent() override = default;

    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(UBillboardComponent, UPrimitiveComponent)

protected:
    //Billboard를 현재 프레임에 렌더할 수 있는지 검사한다.
    bool CanRenderBillBoard() const;

    // Billboard 렌더링에 사용할 World Transform을 반환한다.
    // 기본 구현 : 자신의 ComponentToWorld 사용
    // UNameTagComponent: Target Actor Transform + Offset 사용
    virtual bool TryGetBillBoardWorld(
        FMatrix& OutWorld) const;
};