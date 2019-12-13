/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/


import QtQuick 2.10
import QtQuick.Controls 2.3
import QtQuick.Controls.Material 2.3
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

Item {
    id: id_control
    anchors.fill: parent
    property string labelText: "Tab"
    property string titleText: "Tab Title"
    property bool highlighted : false
}
