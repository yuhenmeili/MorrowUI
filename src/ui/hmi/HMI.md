# 车载组件包括：
## 1. SafeStaticSprite (安全图片组件)
    用于时速、档位、报警灯
    静态 ROM 纹理 + 固定四边形 UV 偏移切换
## 2. SafeStreamTexture (安全流媒体纹理组件)
    SafetyRvcPlayer (倒车影像/CMS 电子后视镜)
    QNX Screen Stream + EGLImage 零拷贝硬解码
## 3. SafeDynamicVectorCanvas  (安全动态矢量画布)
    AsilAdasOverlay 智驾障碍物红框、车道线、倒车动态轨迹线
    静态 VBO 顶点阵列 + 原生几何线条直线绘制
## 4. SafeStaticTextLayout  (安全静态文本排版器)
    SecureOdoComputer 里程表文本、制动/安全带未系等汉字强文字告警
    静态裁剪汉字字集 + 只读固定排版坐标查找表
