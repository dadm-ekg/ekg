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

        // LEWY PANEL – pliki i moduły
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

        // ŚRODKOWY PANEL – wizualizacja
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

        // PRAWY PANEL – parametry analizy
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

                // Karta parametrów – na razie tylko dla ECG BASELINE
                Loader {
                    id: paramsLoader
                    Layout.fillWidth: true
                    sourceComponent: window.currentModule === "ECG BASELINE"
                                     ? baselineParams
                                     : null
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Button {
                        id: runButton
                        text: "Uruchom analizę"
                        Layout.fillWidth: true
                        // tu później podłączymy backend
                    }

                    Button {
                        text: "Reset"
                        Layout.preferredWidth: 100
                        onClicked: {
                            if (paramsLoader.item && window.currentModule === "ECG BASELINE") {
                                paramsLoader.item.resetState()
                            }
                        }
                    }
                }
            }
        }
    }

    // === KOMPONENT PARAMETRÓW DLA ECG BASELINE ===
    Component {
        id: baselineParams

        Rectangle {
            id: baselineRoot
            Layout.fillWidth: true
            radius: 10
            color: "#050d18"
            border.color: "#1f2933"
            border.width: 1
            clip: true
            implicitHeight: baselineColumn.implicitHeight + 20

            // funkcja wołana z przycisku Reset
            function resetState() {
                baselineColumn.selectedFilters = 0
                cbMovingAverage.checked = false
                cbButterworth.checked = false
                cbSavitzky.checked = false
                polyDegreeSpin.value = 3
                showOnlyFiltered.checked = false
            }

            ColumnLayout {
                id: baselineColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                // licznik zaznaczonych filtrów (max 2)
                property int selectedFilters: 0

                function handleToggle(cb) {
                    if (cb.checked) {
                        if (selectedFilters >= 2) {
                            cb.checked = false
                            return
                        }
                        selectedFilters++
                    } else {
                        selectedFilters--
                    }
                }

                Label {
                    text: "ECG BASELINE – ustawienia filtru"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

                Label {
                    text: "Wybierz maksymalnie dwa filtry do porównania:"
                    color: "#9ca3af"
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }

                // Filtry jako checkboxy

                CheckBox {
                    id: cbMovingAverage
                    text: "Moving Average"
                    onToggled: baselineColumn.handleToggle(this)
                }

                CheckBox {
                    id: cbButterworth
                    text: "Butterworth"
                    onToggled: baselineColumn.handleToggle(this)
                }

                CheckBox {
                    id: cbSavitzky
                    text: "Savitzky-Golay"
                    onToggled: baselineColumn.handleToggle(this)
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#1f2933"
                }

                // Stopień wielomianu

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        text: "Stopień wielomianu:"
                        Layout.preferredWidth: 120
                    }

                    SpinBox {
                        id: polyDegreeSpin
                        from: 1
                        to: 10
                        value: 3
                        Layout.fillWidth: true
                    }
                }

                CheckBox {
                    id: showOnlyFiltered
                    text: "Pokaż tylko przefiltrowany sygnał"
                }
            }
        }
    }
}
