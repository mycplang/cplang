# CP 语言图标集

## 文件说明

| 文件 | 用途 | 尺寸 |
|------|------|------|
| `cp-logo.svg` | 主 Logo，深色圆角矩形+CP+语 | 256x256 |
| `cp-file.svg` | .cp 源文件图标，带折角 | 256x256 |
| `cp-icon.svg` | 应用程序图标，圆形 | 256x256 |

## 配色

- 背景: #1a1a2e (深蓝黑)
- 主色: #e94560 (中国红)
- 文字: #ffffff (白)
- 辅助: #7a7a9a (灰)

## 转换

```bash
# 需要 librsvg 或 ImageMagick
rsvg-convert -w 256 cp-logo.svg -o cp-logo.png
convert cp-logo.svg -resize 256x256 cp-logo.png

# Windows .ico (需要 ImageMagick)
convert cp-icon.svg -define icon:auto-resize=256,128,64,48,32,16 cp-icon.ico

# macOS .icns (需要iconutil)
# 先转PNG再打包
```
