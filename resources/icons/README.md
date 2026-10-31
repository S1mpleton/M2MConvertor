# Application icon

The application icon uses the approved M2M family artwork: navy state diagrams
and mint conversion arrows on an ivory rounded square. The matching M2MTools
icon uses a coral pencil to identify the exercise editor.

`m2mconvertor-source.png` is the generated master artwork with transparent outer
corners. `m2mconvertor.png` is the 256 px Qt resource used by the application and
landing screen. `m2mconvertor.ico` contains 16, 24, 32, 48, 64, 128 and 256 px
images and is embedded in the Windows executable.

Format conversion with ImageMagick:

```sh
magick m2mconvertor-source.png -resize 256x256 m2mconvertor.png
magick m2mconvertor-source.png -define icon:auto-resize=256,128,64,48,32,24,16 m2mconvertor.ico
```
