#include "PCH.h"
#include "doctest.h"

#include <rapidjson/document.h>

#include "../Core/Base/TypeRegistry.h"
#include "../Scene/AActor.h"
#include "../Scene/Component/USceneComponent.h"
#include "../Scene/UWorld.h"
#include "../Serialize/FArchiveJson.h"

namespace {
    void RegisterSceneAttachmentTypes() {
        TypeRegistry::Register(AActor::StaticTypeInfo());
        TypeRegistry::Register(USceneComponent::StaticTypeInfo());
    }
}

TEST_SUITE("CH4 Scene Attachment") {
    TEST_CASE("FTransform matrix variants make scale and inverse semantics explicit") {
        const FTransform Transform({ 3.0f, 4.0f, 5.0f }, FRotator::Zero, { 2.0f, 3.0f, 4.0f });
        const FMatrix MatrixWithScale = Transform.ToMatrixWithScale();
        const FMatrix MatrixNoScale = Transform.ToMatrixNoScale();
        const FMatrix InverseMatrixWithScale = Transform.ToInverseMatrixWithScale();

        CHECK(MatrixWithScale.m[0][0] == doctest::Approx(-2.0f));
        CHECK(MatrixWithScale.m[1][2] == doctest::Approx(4.0f));
        CHECK(MatrixWithScale.m[2][1] == doctest::Approx(3.0f));
        CHECK(MatrixNoScale.m[0][0] == doctest::Approx(-1.0f));
        CHECK(MatrixNoScale.m[1][2] == doctest::Approx(1.0f));
        CHECK(MatrixNoScale.m[2][1] == doctest::Approx(1.0f));
        CHECK(MatrixWithScale.Translation() == FVector3(3.0f, 4.0f, 5.0f));

        const FMatrix Identity = MatrixWithScale * InverseMatrixWithScale;
        for (uint32 Row = 0; Row < 4; ++Row) {
            for (uint32 Column = 0; Column < 4; ++Column) {
                CHECK(Identity.m[Row][Column] == doctest::Approx(Row == Column ? 1.0f : 0.0f).epsilon(0.0001f));
            }
        }
    }

    TEST_CASE("FMatrix rotation axes and FRotator yaw use the engine Z-up basis") {
        const FMatrix RotateX = FMatrix::CreateRotationX(DirectX::XM_PIDIV2);
        const FMatrix RotateY = FMatrix::CreateRotationY(DirectX::XM_PIDIV2);
        const FMatrix RotateZ = FMatrix::CreateRotationZ(DirectX::XM_PIDIV2);

        const FVector3 XRotatedY = RotateX.TransformDirection(FVector3::UnitY);
        const FVector3 YRotatedX = RotateY.TransformDirection(FVector3::UnitX);
        const FVector3 YRotatedZ = RotateY.TransformDirection(FVector3::UnitZ);
        const FVector3 ZRotatedX = RotateZ.TransformDirection(FVector3::UnitX);
        CHECK(XRotatedY.x == doctest::Approx(0.0f));
        CHECK(XRotatedY.y == doctest::Approx(0.0f));
        CHECK(XRotatedY.z == doctest::Approx(1.0f));
        CHECK(YRotatedX.x == doctest::Approx(0.0f));
        CHECK(YRotatedX.z == doctest::Approx(-1.0f));
        CHECK(YRotatedZ.x == doctest::Approx(1.0f));
        CHECK(YRotatedZ.z == doctest::Approx(0.0f));
        CHECK(ZRotatedX.x == doctest::Approx(0.0f));
        CHECK(ZRotatedX.y == doctest::Approx(1.0f));
        CHECK(ZRotatedX.z == doctest::Approx(0.0f));

        const FMatrix YawMatrix = FMatrix::CreateFromQuaternion(
            FQuat::FromRotator({ 0.0f, DirectX::XM_PIDIV2, 0.0f }));
        const FVector3 YawRotatedX = YawMatrix.TransformDirection(FVector3::UnitX);
        CHECK(YawRotatedX.x == doctest::Approx(0.0f));
        CHECK(YawRotatedX.y == doctest::Approx(1.0f));
        CHECK(YawRotatedX.z == doctest::Approx(0.0f));
    }

    TEST_CASE("Z-up yaw rotates the camera forward direction in the ground plane") {
        const FTransform CameraAtRest({ 0.0f, 0.0f, 0.0f }, FRotator::Zero, { 1.0f, 1.0f, 1.0f });
        const FTransform CameraYawed({ 0.0f, 0.0f, 0.0f }, { 0.0f, DirectX::XM_PIDIV2, 0.0f }, { 1.0f, 1.0f, 1.0f });

        const FVector3 RestForward = CameraAtRest.ToMatrixNoScale().Forward();
        const FVector3 YawedForward = CameraYawed.ToMatrixNoScale().Forward();
        CHECK(RestForward == FVector3(0.0f, 1.0f, 0.0f));
        CHECK(YawedForward.x == doctest::Approx(-1.0f));
        CHECK(YawedForward.y == doctest::Approx(0.0f));
        CHECK(YawedForward.z == doctest::Approx(0.0f));
    }

    TEST_CASE("Camera pitch follows the yawed local-right quaternion axis") {
        const FQuat Yaw = FQuat::CreateFromAxisAngle(FVector3::UnitZ, DirectX::XM_PIDIV2);
        const FQuat YawedRotation = FQuat::Concatenate(FQuat{}, Yaw);
        const FTransform YawedCamera({ 0.0f, 0.0f, 0.0f }, YawedRotation, { 1.0f, 1.0f, 1.0f });
        const FVector3 PitchAxis = YawedCamera.ToMatrixNoScale().Right();
        const FQuat Pitch = FQuat::CreateFromAxisAngle(PitchAxis, 0.25f);
        const FQuat CameraRotation = FQuat::Concatenate(YawedRotation, Pitch);
        const FTransform Camera({ 0.0f, 0.0f, 0.0f }, CameraRotation, { 1.0f, 1.0f, 1.0f });

        const FVector3 Forward = Camera.ToMatrixNoScale().Forward();
        CHECK(Forward.z > 0.0f);
    }

    TEST_CASE("KeepWorldTransform preserves world location through attach and detach") {
        UWorld World;
        AActor* Actor = World.AdoptActor<AActor>();
        REQUIRE(Actor != nullptr);

        USceneComponent* FirstParent = Actor->AddComponent<USceneComponent>();
        USceneComponent* SecondParent = Actor->AddComponent<USceneComponent>();
        USceneComponent* Child = Actor->AddComponent<USceneComponent>();
        REQUIRE(FirstParent != nullptr);
        REQUIRE(SecondParent != nullptr);
        REQUIRE(Child != nullptr);

        FirstParent->SetRelativeLocation({ 10.0f, 0.0f, 0.0f });
        SecondParent->SetRelativeLocation({ 20.0f, 0.0f, 0.0f });
        Child->SetRelativeLocation({ 5.0f, 0.0f, 0.0f });
        REQUIRE(Child->AttachToComponent(FirstParent));
        CHECK_EQ(Child->GetComponentLocation(), FVector3(15.0f, 0.0f, 0.0f));

        REQUIRE(Child->AttachToComponent(SecondParent, EAttachmentTransformRule::KeepWorldTransform));
        CHECK_EQ(Child->GetParent(), SecondParent);
        CHECK_EQ(Child->GetComponentLocation(), FVector3(15.0f, 0.0f, 0.0f));
        CHECK_EQ(Child->GetRelativeLocation(), FVector3(-5.0f, 0.0f, 0.0f));

        REQUIRE(Child->DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform));
        CHECK_EQ(Child->GetParent(), nullptr);
        CHECK_EQ(Child->GetComponentLocation(), FVector3(15.0f, 0.0f, 0.0f));
        CHECK_EQ(Child->GetRelativeLocation(), FVector3(15.0f, 0.0f, 0.0f));
    }

    TEST_CASE("Actor transform APIs operate on an attached root component in world space") {
        UWorld World;
        AActor* ParentActor = World.AdoptActor<AActor>();
        AActor* ChildActor = World.AdoptActor<AActor>();
        REQUIRE(ParentActor != nullptr);
        REQUIRE(ChildActor != nullptr);

        USceneComponent* Parent = ParentActor->AddComponent<USceneComponent>();
        USceneComponent* Root = ChildActor->AddComponent<USceneComponent>();
        REQUIRE(Parent != nullptr);
        REQUIRE(Root != nullptr);
        REQUIRE(ChildActor->SetRootComponent(Root));

        Parent->SetRelativeLocation({ 100.0f, 0.0f, 0.0f });
        Root->SetRelativeLocation({ 5.0f, 0.0f, 0.0f });
        REQUIRE(Root->AttachToComponent(Parent));
        CHECK_EQ(ChildActor->GetActorLocation(), FVector3(105.0f, 0.0f, 0.0f));

        REQUIRE(ChildActor->SetActorLocation({ 25.0f, 0.0f, 0.0f }));
        CHECK_EQ(ChildActor->GetActorLocation(), FVector3(25.0f, 0.0f, 0.0f));
        CHECK_EQ(Root->GetRelativeLocation(), FVector3(-75.0f, 0.0f, 0.0f));

        REQUIRE(ChildActor->SetActorRelativeLocationAndRotation({ -50.0f, 0.0f, 0.0f }, FRotator::Zero));
        CHECK_EQ(ChildActor->GetActorRelativeLocation(), FVector3(-50.0f, 0.0f, 0.0f));
        CHECK_EQ(ChildActor->GetActorLocation(), FVector3(50.0f, 0.0f, 0.0f));

        REQUIRE(ChildActor->SetActorTransform(FTransform({ 60.0f, 0.0f, 0.0f }, FRotator::Zero, { 1.0f, 1.0f, 1.0f })));
        CHECK_EQ(ChildActor->GetActorTransform().GetLocation(), FVector3(60.0f, 0.0f, 0.0f));
    }

    TEST_CASE("Scene and actor transform APIs expose relative and world rotation and scale") {
        UWorld World;
        AActor* Actor = World.AdoptActor<AActor>();
        REQUIRE(Actor != nullptr);

        USceneComponent* Root = Actor->AddComponent<USceneComponent>();
        REQUIRE(Root != nullptr);
        REQUIRE(Actor->SetRootComponent(Root));

        Root->SetRelativeRotation({ 0.25f, -0.5f, 0.75f });
        Root->SetRelativeScale3D({ 2.0f, 3.0f, 4.0f });
        CHECK_EQ(Root->GetRelativeRotation(), FVector3(0.25f, -0.5f, 0.75f));
        CHECK_EQ(Root->GetRelativeScale3D(), FVector3(2.0f, 3.0f, 4.0f));

        REQUIRE(Actor->SetActorRotation(FRotator::Zero));
        REQUIRE(Actor->SetActorScale3D({ 1.5f, 2.0f, 2.5f }));

        const FRotator WorldRotation = Actor->GetActorRotation();
        const FVector3 WorldScale = Actor->GetActorScale3D();
        CHECK(WorldRotation.x == doctest::Approx(0.0f).epsilon(0.0001f));
        CHECK(WorldRotation.y == doctest::Approx(0.0f).epsilon(0.0001f));
        CHECK(WorldRotation.z == doctest::Approx(0.0f).epsilon(0.0001f));
        CHECK(WorldScale.x == doctest::Approx(1.5f).epsilon(0.0001f));
        CHECK(WorldScale.y == doctest::Approx(2.0f).epsilon(0.0001f));
        CHECK(WorldScale.z == doctest::Approx(2.5f).epsilon(0.0001f));

        REQUIRE(Root->SetWorldLocationAndRotation({ 10.0f, 20.0f, 30.0f }, FRotator::Zero));
        CHECK_EQ(Actor->GetActorLocation(), FVector3(10.0f, 20.0f, 30.0f));
    }

    TEST_CASE("Attached transforms compose quaternion rotation and scale without shear") {
        UWorld World;
        AActor* Actor = World.AdoptActor<AActor>();
        REQUIRE(Actor != nullptr);

        USceneComponent* Parent = Actor->AddComponent<USceneComponent>();
        USceneComponent* Child = Actor->AddComponent<USceneComponent>();
        REQUIRE(Parent != nullptr);
        REQUIRE(Child != nullptr);

        Parent->SetRelativeTransform({
            { 10.0f, 20.0f, 30.0f }, { 0.25f, -0.5f, 0.75f }, { 2.0f, 3.0f, 4.0f }
        });
        Child->SetRelativeTransform({
            { 5.0f, -2.0f, 1.0f }, { -0.4f, 0.3f, -0.2f }, { 5.0f, 6.0f, 7.0f }
        });
        REQUIRE(Child->AttachToComponent(Parent));

        const FTransform Expected = Child->GetRelativeTransform().Compose(Parent->GetRelativeTransform());
        const FTransform Actual = Child->GetComponentTransform();
        CHECK(Actual.GetLocation().x == doctest::Approx(Expected.GetLocation().x).epsilon(0.0001f));
        CHECK(Actual.GetLocation().y == doctest::Approx(Expected.GetLocation().y).epsilon(0.0001f));
        CHECK(Actual.GetLocation().z == doctest::Approx(Expected.GetLocation().z).epsilon(0.0001f));
        CHECK_EQ(Actual.GetScale(), FVector3(10.0f, 18.0f, 28.0f));

        const FMatrix WorldMatrix = Child->GetComponentToWorld();
        FVector3 Right = WorldMatrix.Right();
        FVector3 Up = WorldMatrix.Up();
        FVector3 Forward = WorldMatrix.Forward();
        Right.Normalize();
        Up.Normalize();
        Forward.Normalize();
        CHECK(Right.Dot(Up) == doctest::Approx(0.0f).epsilon(0.0001f));
        CHECK(Right.Dot(Forward) == doctest::Approx(0.0f).epsilon(0.0001f));
        CHECK(Up.Dot(Forward) == doctest::Approx(0.0f).epsilon(0.0001f));
    }

    TEST_CASE("Attached transforms can inherit location rotation and scale independently") {
        UWorld World;
        AActor* Actor = World.AdoptActor<AActor>();
        REQUIRE(Actor != nullptr);

        USceneComponent* Parent = Actor->AddComponent<USceneComponent>();
        USceneComponent* Child = Actor->AddComponent<USceneComponent>();
        REQUIRE(Parent != nullptr);
        REQUIRE(Child != nullptr);

        Parent->SetRelativeTransform({
            { 10.0f, 20.0f, 30.0f }, { 0.2f, -0.4f, 0.6f }, { 2.0f, 3.0f, 4.0f }
        });
        FTransform ChildTransform{
            { 5.0f, 6.0f, 7.0f }, { -0.3f, 0.1f, 0.5f }, { 5.0f, 6.0f, 7.0f }
        };
        ChildTransform.SetAbsoluteRotation(true);
        Child->SetRelativeTransform(ChildTransform);
        REQUIRE(Child->AttachToComponent(Parent));

        const FTransform RotationAbsoluteWorld = Child->GetComponentTransform();
        const FQuat ChildRotation = Child->GetRelativeTransform().GetRotationQuaternion();
        const FQuat WorldRotation = RotationAbsoluteWorld.GetRotationQuaternion();
        const float RotationDot = std::abs(
            ChildRotation.x * WorldRotation.x + ChildRotation.y * WorldRotation.y +
            ChildRotation.z * WorldRotation.z + ChildRotation.w * WorldRotation.w);
        CHECK(RotationDot == doctest::Approx(1.0f).epsilon(0.0001f));
        CHECK_EQ(RotationAbsoluteWorld.GetScale(), FVector3(10.0f, 18.0f, 28.0f));

        ChildTransform.SetAbsoluteLocation(true);
        ChildTransform.SetAbsoluteScale(true);
        Child->SetRelativeTransform(ChildTransform);
        const FTransform FullyAbsoluteWorld = Child->GetComponentTransform();
        CHECK_EQ(FullyAbsoluteWorld.GetLocation(), FVector3(5.0f, 6.0f, 7.0f));
        CHECK_EQ(FullyAbsoluteWorld.GetScale(), FVector3(5.0f, 6.0f, 7.0f));
    }

    TEST_CASE("Scene component parent references survive serialization and resolve") {
        RegisterSceneAttachmentTypes();
        rapidjson::Document Document;
        Document.SetObject();

        {
            UWorld SourceWorld;
            AActor* SourceActor = SourceWorld.AdoptActor<AActor>();
            REQUIRE(SourceActor != nullptr);

            USceneComponent* SourceParent = SourceActor->AddComponent<USceneComponent>();
            USceneComponent* SourceChild = SourceActor->AddComponent<USceneComponent>();
            SourceParent->SetRelativeLocation({ 10.0f, 0.0f, 0.0f });
            SourceChild->SetRelativeLocation({ 5.0f, 0.0f, 0.0f });
            REQUIRE(SourceChild->AttachToComponent(SourceParent));

            FArchiveJson ArchiveSave(Document, Document.GetAllocator());
            SourceActor->Save(ArchiveSave);
        }

        AActor LoadedActor;
        FArchiveJson ArchivePreLoad(Document);
        REQUIRE(LoadedActor.PreLoadComponents(ArchivePreLoad));

        FArchiveJson ArchiveLoad(Document);
        LoadedActor.Load(ArchiveLoad);
        REQUIRE(LoadedActor.ResolveLoadedReferences());

        REQUIRE_EQ(LoadedActor.GetComponents().size(), 2);
        USceneComponent* LoadedParent = static_cast<USceneComponent*>(LoadedActor.GetComponents()[0].get());
        USceneComponent* LoadedChild = static_cast<USceneComponent*>(LoadedActor.GetComponents()[1].get());

        CHECK_EQ(LoadedChild->GetParent(), LoadedParent);
        CHECK_EQ(LoadedParent->GetChildren().size(), 1);
        CHECK_EQ(LoadedChild->GetRelativeLocation(), FVector3(5.0f, 0.0f, 0.0f));
        CHECK_EQ(LoadedChild->GetComponentLocation(), FVector3(15.0f, 0.0f, 0.0f));
    }
}
