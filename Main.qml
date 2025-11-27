import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

ApplicationWindow {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "EKG Analyzer"
    color: "#020712"

    Material.theme: Material.Dark
    Material.accent: Material.Teal

    property string currentModule: "ECG BASELINE"

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 16

            Label {
                text: "EKG Analyzer"
                font.pixelSize: 22
                font.bold: true
                Layout.alignment: Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true }

            Button { text: "Import sygnału" }
            Button { text: "Zapisz wyniki" }
            Button { text: "Ustawienia" }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        anchors.topMargin: header.height + 16
        spacing: 16

        //LEWY PANEL – pliki i moduły
        Frame {
            Layout.preferredWidth: 260
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Label {
                    text: "Pliki / badania"
                    font.bold: true
                    font.pixelSize: 16
                }

                TextField {
                    placeholderText: "Filtruj listę..."
                    Layout.fillWidth: true
                }

                ListView {
                    id: fileList
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220
                    clip: true
                    model: 5
                    delegate: ItemDelegate {
                        width: ListView.view.width
                        text: "record_" + (index + 1) + ".dat"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#1f2933"
                }

                Label {
                    text: "Moduł analizy"
                    font.bold: true
                    font.pixelSize: 16
                }

                ComboBox {
                    id: moduleCombo
                    Layout.fillWidth: true
                    model: [
                        "ECG BASELINE",
                        "R PEAKS",
                        "WAVES",
                        "HRV1",
                        "HRV2",
                        "HRV DFA",
                        "HEART CLASS"
                    ]
                    onCurrentTextChanged: window.currentModule = currentText
                }
            }
        }

        //ŚRODKOWY PANEL – wizualizacja
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Label {
                    text: "Wizualizacja sygnału"
                    font.bold: true
                    font.pixelSize: 16
                }

                Rectangle {
                    id: visualizationArea
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 10
                    color: "#020812"
                    border.color: "#0f172a"
                    border.width: 1

                    Label {
                        anchors.centerIn: parent
                        text: "Tutaj moduł Visualization wstawi wykresy EKG."
                        color: "#6b7280"
                    }
                }
            }
        }

        //PRAWY PANEL – parametry analizy
        Frame {
            Layout.preferredWidth: 380
            Layout.maximumWidth: 420
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Label {
                    text: "Panel analizy"
                    font.bold: true
                    font.pixelSize: 16
                }

                Label {
                    text: "Aktywny moduł: " + window.currentModule
                    color: "#9ca3af"
                    wrapMode: Text.WordWrap
                }

                // Karta parametrów filtru
                Rectangle {
                    Layout.fillWidth: true
                    radius: 10
                    color: "#050d18"
                    border.color: "#1f2933"
                    border.width: 1
                    clip: true
                    implicitHeight: paramsColumn.implicitHeight + 20

                    ColumnLayout {
                        id: paramsColumn
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        Label {
                            text: "ECG BASELINE – ustawienia filtru"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Label {
                                text: "Metoda filtru:"
                                Layout.preferredWidth: 120
                            }

                            ComboBox {
                                id: filterMethodCombo
                                Layout.fillWidth: true
                                model: [ "Moving Average", "Butterworth", "Savitzky-Golay" ]
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Label {
                                text: "Fc [Hz]:"
                                Layout.preferredWidth: 120
                            }

                            Slider {
                                id: cutoffSlider
                                from: 0.1
                                to: 5.0
                                value: 0.5
                                stepSize: 0.1
                                Layout.fillWidth: true
                            }

                            Label {
                                text: cutoffSlider.value.toFixed(1)
                                width: 36
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Label {
                                text: "Rząd filtru:"
                                Layout.preferredWidth: 120
                            }

                            SpinBox {
                                id: orderSpin
                                from: 1
                                to: 8
                                value: 4
                                Layout.fillWidth: true
                            }
                        }

                        CheckBox {
                            id: showOnlyFiltered
                            text: "Pokaż tylko przefiltrowany sygnał"
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Button {
                        id: runButton
                        text: "Uruchom analizę"
                        Layout.fillWidth: true
                    }

                    Button {
                        text: "Reset"
                        Layout.preferredWidth: 100
                        onClicked: {
                            filterMethodCombo.currentIndex = 0
                            cutoffSlider.value = 0.5
                            orderSpin.value = 4
                            showOnlyFiltered.checked = false
                        }
                    }
                }
            }
        }
    }
}
