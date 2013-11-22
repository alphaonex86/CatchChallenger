import QtQuick 2.1;
import QtQuick.Particles 2.0;
import QtQuick.Controls 1.0;
import QtQuick.Controls.Styles 1.0;

Rectangle {
    id: win;
    width: 800;
    height: 600;    
    Image {
        source: "qrc:/images/interface/player/background.png";
        smooth: false;
        anchors {
            left: parent.left;
            bottom: parent.bottom
        }
    }
    Timer {
        interval: 5000
        repeat: false
        running: true
        triggeredOnStart: false
        onTriggered: {
            animationControl.animationFinished();
        }
    }
    Image {
        id: fromMonster;
        source: baseMonsterEvolution.image();
	smooth: false;
        width: (sourceSize.width * 2);
        height: width;
        sourceSize.width: 80;
        sourceSize.height: 80;
	anchors.centerIn: parent;
	NumberAnimation on opacity {
            running: true;
            loops: 1;
            from: 1.0;
            to: 0.0;
            duration: 5000;
        }
    }
    Image {
        id: toMonster;
        source: targetMonsterEvolution.image();
	smooth: false;
        width: (sourceSize.width * 2);
        height: width;
        sourceSize.width: 80;
        sourceSize.height: 80;
	anchors.centerIn: parent;
	NumberAnimation on opacity {
            running: true;
            loops: 1;
            from: 0.0;
            to: 1.0;
            duration: 5000;
        }
    }
    Text {
	text: evolutionControl.evolutionText();
	color: "black"
	anchors.horizontalCenter: parent.horizontalCenter
	y:500;
	font { pixelSize: 20; }
    }
Button {
	id: button
	anchors.horizontalCenter: parent.horizontalCenter
	y:550;
	text: "Cancel"
	onClicked: evolutionControl.cancel();
	visible: canBeCanceled;
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
