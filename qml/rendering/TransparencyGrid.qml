pragma ComponentBehavior: Bound
import QtQuick

// Editor/thumbnail guidance only; never enable this in an audience or NDI scene.
Canvas {
    property bool dark: LibraryStyle.dark

    onDarkChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        const ctx = getContext("2d"), size = 32;
        ctx.fillStyle = dark ? "#292929" : "#EEEEEE";
        ctx.fillRect(0, 0, width, height);
        ctx.fillStyle = dark ? "#383838" : "#D1D1D1";
        for (let y = 0; y < height; y += size)
            for (let x = 0; x < width; x += size)
                if ((Math.floor(x / size) + Math.floor(y / size)) % 2)
                    ctx.fillRect(x, y, size, size);
    }
}
