# Offline IBL Bake Output

Source HDR: `../assets/textures/hdr/symmetrical_garden_1k.hdr`

## Files
- `irradiance_equirect.png`: RGBM encoded diffuse irradiance equirect map
- `specular_equirect.png`: RGBM encoded GGX prefiltered specular atlas
- `brdf_lut.png`: split-sum BRDF LUT, BRDF.x in R, BRDF.y in G

## RGBM decode
```glsl
vec3 decodeRGBM(vec4 rgbm) {
    return rgbm.rgb * (rgbm.a * 64);
}
```

## Specular atlas layout
Mip levels are stacked from top to bottom.

| mip | roughness | width | height | offsetY |
| --- | --------- | ----- | ------ | ------- |
| 0 | 0 | 512 | 256 | 0 |
| 1 | 0.25 | 256 | 128 | 256 |
| 2 | 0.5 | 128 | 64 | 384 |
| 3 | 0.75 | 64 | 32 | 448 |
| 4 | 1 | 32 | 16 | 480 |
