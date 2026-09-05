# Particles2DDemo

- 对应源码：`samples/Particles2DDemo.cpp`
- 编译目标：`Particles2DDemo`

## Demo 用途

对比展示两种 2D 粒子组件：左侧 `MRCPUParticles2D`（CPU 更新，暖色飘落粒子），
右侧 `MRGPUParticles2D`（GPU 更新，冷色发光氛围粒子），加法混合营造夜景氛围。

## 运行方式

```bash
cmake --build build --target Particles2DDemo --parallel 8
./build/Particles2DDemo.exe
```

## 示例做了什么

1. 深蓝夜色背景，左右两块 `MRColor` 圆角面板 + `MRLabel` 标题。
2. **CPU 粒子**：`setAmount(180)`、`setLifetime(5)`、`setEmissionSize`（顶部条带发射区）、
   `setVelocityRange`（向下速度区间）、`setGravity(0, -12)`、`setSizeRange(5, 12)`、
   `setColorGradient(暖黄 → 红, alpha 0.95 → 0)`、`setAdditiveBlend(true)`。
3. **GPU 粒子**：`setAmount(900)`（GPU 路径量级明显更大）、区域全面板发射、
   四向速度范围、`setParticleSize(13)`、`setColorGradient(青 → 紫)`、加法混合。

## 相关组件

### `MRCPUParticles2D`
- CPU 粒子系统：每帧在 CPU 更新粒子并写入顶点缓冲，粒子量适合数百级，
  参数接口完整（发射区/速度/重力/尺寸/颜色渐变/混合）。

### `MRGPUParticles2D`
- GPU 粒子系统：位置积分在顶点着色器完成（`particle_gpu` shader），CPU 只上传
  发射参数，适合千级以上的氛围粒子；接口与 CPU 版本风格一致。

### 加法混合
- `setAdditiveBlend(true)` 切换材质混合模式为加法，暗背景下营造发光叠加效果。
