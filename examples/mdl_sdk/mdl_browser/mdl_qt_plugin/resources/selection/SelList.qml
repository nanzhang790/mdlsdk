/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/


import QtQuick 2.10
import QtQuick.Controls 2.3
import QtQuick.Controls.Material 2.3
import QtQuick.Layouts 1.3

import "../search" as Search

ColumnLayout {
    id: id_control

    property real scrollBarWidth: 7.5
    width: parent.width
    height: parent.height

    property int elementHeight: 96
    property int sideMargin: 30
    property int topMargin: 15
    property int bottomMargin: 10

    property var model: sel_mockup

    Connections {
        target: model
        // clear selection when the filter changed
        onFiltering_about_to_start: id_list.currentIndex = -1
    }

    function clearSelection() {
        id_list.currentIndex = -1
    }

    ListView {
        id: id_list
        anchors.fill: parent
        anchors.leftMargin: sideMargin
        anchors.rightMargin: sideMargin

        currentIndex: -1
    
        header: Item {
            height: id_control.topMargin
        }
        footer: Item {
            height: id_control.bottomMargin
        }

        focus: true
        clip: true

        model: id_control.model

        delegate: SelListItem {
            width: (id_control.width - 2 * sideMargin)
            targetHeight: elementHeight
            isSelected: id_list.currentIndex == index

            onClicked: {
                if(id_list.currentIndex == index)
                    id_list.currentIndex = -1
                else
                    id_list.currentIndex = index
            }

            onConfirmed: {
                vm_mdl_browser.set_result_and_close(value);
            }
        }

        ScrollBar.vertical: ScrollBar {
            parent: id_control

            anchors.top: id_list.top
            anchors.left: id_list.right
            anchors.leftMargin: 5
            anchors.bottom: id_list.bottom

            implicitWidth: scrollBarWidth
        }
    }
}
