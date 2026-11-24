pragma Singleton
import QtQuick
import QtCore
import QtMultimedia

// Operator feedback only. These effects never enter the slide/media audio pipeline.
QtObject {
    id: feedback

    property Settings preferences: Settings {
        id: stored

        property bool sounds: true
        property bool reducedMotion: false

        category: "InteractionFeedback"
    }
    readonly property bool soundEnabled: stored.sounds
    readonly property bool reducedMotion: stored.reducedMotion
    readonly property int duration: reducedMotion ? 0 : 160
    property bool presentationMode: false
    property double lastPlayed: 0
    readonly property bool soundReady: clickSound.status === SoundEffect.Ready
    property SoundEffect clickSound: SoundEffect {
        source: "qrc:/sounds/click.wav"
        volume: 0.8
    }
    property SoundEffect selectSound: SoundEffect {
        source: "qrc:/sounds/select.wav"
        volume: 0.8
    }
    property SoundEffect toggleSound: SoundEffect {
        source: "qrc:/sounds/toggle.wav"
        volume: 0.8
    }
    property SoundEffect openSound: SoundEffect {
        source: "qrc:/sounds/switch.wav"
        volume: 0.8
    }
    property SoundEffect closeSound: SoundEffect {
        source: "qrc:/sounds/back.wav"
        volume: 0.8
    }
    property SoundEffect dropSound: SoundEffect {
        source: "qrc:/sounds/drop.wav"
        volume: 0.8
    }
    property SoundEffect successSound: SoundEffect {
        source: "qrc:/sounds/pluck.wav"
        volume: 0.8
    }
    property SoundEffect tickSound: SoundEffect {
        source: "qrc:/sounds/tick.wav"
        volume: 0.8
    }

    function setSounds(enabled) {
        stored.sounds = enabled;
    }

    function setReducedMotion(enabled) {
        stored.reducedMotion = enabled;
    }

    function play(kind = "click") {
        if (!soundEnabled || presentationMode || Date.now() - lastPlayed < 70)
            return;
        const cues = {
            click: clickSound,
            select: selectSound,
            toggle: toggleSound,
            open: openSound,
            close: closeSound,
            drop: dropSound,
            success: successSound,
            tick: tickSound
        };
        const sound = cues[kind] || clickSound;
        if (sound.status !== SoundEffect.Ready)
            return;
        lastPlayed = Date.now();
        sound.play();
    }
}
