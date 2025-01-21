# RavaEngine

<p align="center">
  <img width="512" height="512" src="https://github.com/ricky10099/RavaEngine/blob/main/Assets/System/Rava.png">
</p>

Self learning Vulkan API and try to make a game engine.

The name "Rava" takes Lava which related to Vulkan as volcano, and with the R of Ricky which is my name.

Vulkanを独学していて、ゲームエンジンを作ってみます。

"Rava"という名前はLavaとRickyの頭文字を組み合わせです。

## Start
<ins>**1. Download the repository:**</ins>

Clone the repository with `--recursive` to ensure cloning with all the submodules. 

すべてのサブモジュールを確実にクローンするため、`--recursive`でリポジトリをクローンする。

```
git clone --recursive https://github.com/ricky10099/RavaEngine.git
```

If the repository was not cloned with `--recursive`, use `git submodule update --init` to clone the submodules.

`recursive`を使わずにリポジトリをクローンした場合は、`git submodule update --init` を使ってサブモジュールをクローンする。

<ins>**2. Create project:**</ins>

Run `CreateProject.bat` to create project.

`CreateProject.bat`を実行してプロジェクトを作成する。

## Compiling Shader
The `CompileShader` in root folder is for Visual Studio post build event.

If you want to compile shader manually, run the `CompileShader.bat` in `RavaEngineCore` folder.

ルートフォルダの `CompileShader` は Visual Studio のポストビルドイベント用です。

シェーダを手動でコンパイルする場合は、`RavaEngineCore`フォルダ内の`CompileShader.bat`を実行する。

## External libraries used:
* [Vulkan](https://vulkan.lunarg.com/): Graphic API for rendering | レンダリング用グラフィックAPI
* [GLFW](https://www.glfw.org/): Create window and handle input | ウィンドウの制御と入力の処理
* [glm](https://github.com/g-truc/glm): Mathematics library | 数学ライブラリ
* [Dear ImGui](https://github.com/ocornut/imgui) GUI library | GUIライブラリ
* [stb_image.h](https://github.com/nothings/stb): Load image | 画像の読み込み
* [ufbx](https://github.com/ufbx/ufbx) Load model | モデルの読み込み
* [PhysX](https://github.com/NVIDIA-Omniverse/PhysX) Physics engine | 物理演算エンジン
* [CRI ADX LE](https://game.criware.jp/) Audio data storage and playback | 音声データの格納と再生
