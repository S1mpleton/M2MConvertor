# Application icon

The application icon uses the approved M2M family artwork: navy state diagrams
and mint conversion arrows on an ivory rounded square. The matching M2MTools
icon uses a coral pencil to identify the exercise editor.

`m2mconvertor-source.png` is the generated master artwork with transparent outer
corners. `m2mconvertor.png` is the 256 px Qt resource used by the application and
landing screen.

`m2mconvertor-small-source.png` is the compact drawing for window title bars,
with a larger pictogram, thicker strokes and less whitespace. The 16, 20, 24
and 32 px PNG resources use this drawing; Qt selects the matching size for the
screen scale. Windows controls the title-bar icon dimensions.

`m2mconvertor.ico` combines the compact drawing at 16–32 px and the standard
drawing at 40, 48, 64, 128 and 256 px. It is embedded in the Windows executable.

Format conversion with ImageMagick:

```sh
magick m2mconvertor-source.png -resize 256x256 m2mconvertor.png
for icon_size in 16 20 24 32; do
    magick m2mconvertor-small-source.png -resize "${icon_size}x${icon_size}" "m2mconvertor-${icon_size}.png"
done
magick m2mconvertor-16.png m2mconvertor-20.png m2mconvertor-24.png m2mconvertor-32.png \
    \( m2mconvertor.png -resize 40x40 \) \( m2mconvertor.png -resize 48x48 \) \
    \( m2mconvertor.png -resize 64x64 \) \( m2mconvertor.png -resize 128x128 \) \
    m2mconvertor.png m2mconvertor.ico
```
