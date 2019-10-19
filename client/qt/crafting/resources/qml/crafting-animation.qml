import QtQuick 2.1;
import QtQuick.Particles 2.0;

Rectangle {
    id: win;
    width: 800;
    height: 600;

    readonly
    property real fullCircle : (Math.PI * 2);
    property real offsetX    : (width / 5);
    property real offsetY    : (height / 5);

    Image {
        source: "qrc:/CC/images/interface/player/background.jpg";
        smooth: false;
        anchors {
            left: parent.left;
            bottom: parent.bottom
        }
    }

    Timer {
        interval: 5500
        repeat: false
        running: true
        triggeredOnStart: false
        onTriggered: {
            recipeImage.opacity=0;
            ingredientsList.opacity=0;
            resultFadeIn.start();
        }
    }
    Timer {
        interval: 8000
        repeat: false
        running: true
        triggeredOnStart: false
        onTriggered: {
            particles.stop();
        }
    }
    Timer {
        interval: 8000
        repeat: false
        running: true
        triggeredOnStart: false
        onTriggered: {
            animationControl.animationFinished();
        }
    }
    Timer {
        interval: 5000
        repeat: false
        running: true
        triggeredOnStart: false
        onTriggered: {
            pulseEmitter.pulse(500);
        }
    }
    Image {
        id: backImage;
        source: craftingAnimationObject.backPlayer();
        smooth: false;
        width: (sourceSize.width * 2);
        height: (sourceSize.height * 2);
        sourceSize.width: 80;
        sourceSize.height: 80;
        anchors {
            left: parent.left;
            bottom: parent.bottom
            leftMargin: deltaX;
        }

        property real deltaX : 1000;

        NumberAnimation {
            target: backImage;
            property: "deltaX";
            running: true;
            to: 100;
            duration: 1000;
        }
    }
    Image {
        id: recipeImage;
        source: craftingAnimationObject.recipe();
        smooth: false;
        width: (sourceSize.width * 2);
        height: width;
        sourceSize.width: 24;
        sourceSize.height: 24;
        anchors.centerIn: parent;
    }
    Image {
        id: resultProduct;
        source: craftingAnimationObject.product();
        smooth: false;
        width: (sourceSize.width * 2);
        height: width;
        sourceSize.width: 24;
        sourceSize.height: 24;
        anchors.centerIn: parent;
    opacity: 0.0;
    NumberAnimation on opacity {
            id: resultFadeIn;
            running: false;
            loops: 1;
            from: 0.0;
            to: 1.0;
            duration: 500;
        }
    }
    Item {
        id: ingredientsList;
        anchors.centerIn: parent;

        Repeater {
            id: monRepeater
            model: craftingAnimationObject.ingredients();/*[
                { "start": 0.00, "end": 1.00 , "src": "3.png" },
                { "start": 0.33, "end": 1.33 , "src": "bottle-empty.png" },
                { "start": 0.66, "end": 1.66 , "src": "sugar.png" }
            ];*/
            delegate: Image {
                id: ball;
        source: modelData;
        smooth: false;
        width: (sourceSize.width * 2);
        height: width;
        sourceSize.width: 24;
        sourceSize.height: 24;
                anchors {
                    centerIn: parent;
                    verticalCenterOffset: (offsetY * offsetCenter * Math.cos (deltaT * fullCircle));
                    horizontalCenterOffset: (offsetX * offsetCenter * Math.sin (deltaT * fullCircle));
                }

                property real deltaT  : 0.0;
        property real offsetCenter  : 1.0;
        Timer {
            interval: 5500
            repeat: false
            running: true
            triggeredOnStart: false
            onTriggered: {
            ingredientsListCircle.stop();
            }
        }
                NumberAnimation {
                id: ingredientsListCircle;
                    target: ball;
                    property: "deltaT";
                    running: true;
                    alwaysRunToEnd: true;
                    loops: 3;
                    from: 1.0/monRepeater.count*index;//modelData['start'];
                    to: 1.0/monRepeater.count*index+1.0;
                    duration: 3000;
                }
                NumberAnimation {
                    target: ball;
                    property: "offsetCenter";
                    running: true;
                    alwaysRunToEnd: true;
                    loops: 1;
                    from: 1.0;
                    to: 0.0;
                    duration: 5000;
                }
            }
        }
    }
    ParticleSystem {
        id: particles
        anchors.fill: parent
        ImageParticle {
            source: "star.png"
            alpha: 0
            colorVariation: 0.6
        }
        Emitter {
            id: pulseEmitter
            x: parent.width/2
            y: parent.height/2
            emitRate: 1000
            lifeSpan: 2000
            enabled: false
            velocity: AngleDirection{magnitude: 64; angleVariation: 360}
            size: 24
            sizeVariation: 8
        }
    }
    Rectangle {
        color: "black";
        anchors.fill: parent;

        NumberAnimation on opacity {
            running: true;
            loops: 1;
            from: 1.0;
            to: 0.0;
            duration: 500;
        }
    }
}
