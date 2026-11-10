# Variant 01 — active application icon

This is the first approved icon pair: a white M2M monogram across a diagonal
charcoal/silver square. M2MConvertor uses blue conversion arrows and state
diagrams; M2MTools uses a blue reaction sheet and pencil.

`m2mconvertor-source.png` is the master artwork. `m2mconvertor.png` is the 256 px
Qt resource used in the window and landing screen. The 16, 20, 24 and 32 px PNG
files serve native small-icon sizes; `m2mconvertor.ico` also contains 40, 48, 64,
128 and 256 px frames for Windows.

The previous state-graph icon set is preserved unchanged in the parent directory.

Regenerate the exported formats with ImageMagick:

```sh
magick m2mconvertor-source.png -resize 256x256 m2mconvertor.png
for icon_size in 16 20 24 32; do
    magick m2mconvertor-source.png -resize "${icon_size}x${icon_size}" "m2mconvertor-${icon_size}.png"
done
magick m2mconvertor-16.png m2mconvertor-20.png m2mconvertor-24.png m2mconvertor-32.png \
    \( m2mconvertor.png -resize 40x40 \) \( m2mconvertor.png -resize 48x48 \) \
    \( m2mconvertor.png -resize 64x64 \) \( m2mconvertor.png -resize 128x128 \) \
    m2mconvertor.png m2mconvertor.ico
```
