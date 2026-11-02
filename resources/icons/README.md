# Application icon

The active application icon is [variant 01](variant-01/README.md): the diagonal
charcoal/silver M2M monogram with blue conversion arrows and state diagrams.
Its Qt resources and Windows ICO are stored in `variant-01/`.

## Preserved state-graph set

The previous application icon files remain here, unchanged and tracked in Git.
They use the approved M2M family artwork: navy state diagrams
and mint conversion arrows on an ivory rounded square. The matching M2MTools
icon uses a coral pencil to identify the exercise editor.

`m2mconvertor-source.png` is the generated master artwork with transparent outer
corners. `m2mconvertor.png` is the previous 256 px Qt resource.

`m2mconvertor-small-source.png` is the compact drawing for window title bars,
with a larger pictogram, thicker strokes and less whitespace. The 16, 20, 24
and 32 px PNG resources use this drawing; Qt selects the matching size for the
screen scale. Windows controls the title-bar icon dimensions.

`m2mconvertor.ico` combines the compact drawing at 16–32 px and the standard
drawing at 40, 48, 64, 128 and 256 px. This previous Windows icon is retained as an alternative.

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
