#include "AVImageLoader.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"

UTexture2D* FAVImageLoader::LoadPNG(const FString& RelativeContentPath, bool bPixelArt)
{
    // Block path traversal: data must come from the game's Content directory.
    if(RelativeContentPath.Contains(TEXT("..")) || !RelativeContentPath.StartsWith(TEXT("AVVisuals/")))
        return nullptr;
    TArray<uint8> Compressed;
    const FString Filename=FPaths::Combine(FPaths::ProjectContentDir(),RelativeContentPath);
    if(!FFileHelper::LoadFileToArray(Compressed,*Filename) || Compressed.Num()==0)
        return nullptr;

    IImageWrapperModule& Module=FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
    TSharedPtr<IImageWrapper> PNG=Module.CreateImageWrapper(EImageFormat::PNG);
    if(!PNG.IsValid() || !PNG->SetCompressed(Compressed.GetData(),Compressed.Num()))
        return nullptr;

    const int32 W=PNG->GetWidth(),H=PNG->GetHeight();
    if(W<=0 || H<=0 || W>4096 || H>4096) return nullptr;
    TArray<uint8> Pixels;
    if(!PNG->GetRaw(ERGBFormat::BGRA,8,Pixels) || Pixels.Num()!=W*H*4)
        return nullptr;
    UTexture2D* Texture=UTexture2D::CreateTransient(W,H,PF_B8G8R8A8);
    if(!Texture || !Texture->PlatformData || Texture->PlatformData->Mips.Num()==0)
        return nullptr;
    Texture->Filter=bPixelArt?TF_Nearest:TF_Bilinear;
    Texture->SRGB=true;
    Texture->NeverStream=true;
    Texture->CompressionSettings=TC_Default;
    void* Data=Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    if(Data) FMemory::Memcpy(Data,Pixels.GetData(),Pixels.Num());
    Texture->PlatformData->Mips[0].BulkData.Unlock();
    if(!Data) return nullptr;
    Texture->UpdateResource();
    return Texture;
}
