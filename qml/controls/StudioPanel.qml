pragma ComponentBehavior: Bound
import QtQuick

import QtQuick.Effects

Rectangle {
    color: LibraryStyle.tone("#191919")
    border.color: LibraryStyle.tone("#343434")
    radius: 9
    layer.enabled: true

    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowOpacity: 0.3
        shadowBlur: 0.5
        shadowVerticalOffset: 8
    }
}
