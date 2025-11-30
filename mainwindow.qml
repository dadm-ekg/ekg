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




    //sterowanie motywem
    property bool isDarkTheme: true

    //kolory zależne od motywu
    property color bgMain:        isDarkTheme ? "#020712" : "#f3f4f6"
    property color panelColor:    isDarkTheme ? "#050d18" : "#ffffff"
    property color borderColor:   isDarkTheme ? "#1f2933" : "#d1d5db"
    property color textSecondary: isDarkTheme ? "#9ca3af" : "#4b5563"
    property color vizBg:         isDarkTheme ? "#020812" : "#f9fafb"
    property color vizBorder:     isDarkTheme ? "#0f172a" : "#d1d5db"

    //kolor tekstu / ikon w przyciskach na pasku
    property color buttonTextColor: isDarkTheme ? "#f9fafb" : "#111827"


    color: bgMain
    Material.theme: isDarkTheme ? Material.Dark : Material.Light
    Material.accent: Material.Teal

    property string currentModule: "ECG BASELINE"

    Connections {
        target: ekgController
        function onFileLoadSuccess(filename) {
            analysisStatus.text = "Status: załadowano plik " + filename.split('/').pop()
            analysisStatus.color = Material.color(Material.Green)
        }
        function onFileLoadError(errorMessage) {
            analysisStatus.text = "Status: błąd - " + errorMessage
            analysisStatus.color = Material.color(Material.Red)
        }
        function onFilteringSuccess(filterName) {
            analysisStatus.text = "Status: filtrowanie " + filterName + " zakończone pomyślnie"
            analysisStatus.color = Material.color(Material.Green)
            fakeProgress.stop()
            analysisProgress.value = 100
        }
        function onFilteringError(errorMessage) {
            analysisStatus.text = "Status: " + errorMessage
            analysisStatus.color = Material.color(Material.Red)
            fakeProgress.stop()
            analysisProgress.value = 0
        }
    }

    header: ToolBar {
        leftPadding: 8
        rightPadding: 8
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

            Button {
                text: "Import sygnału"
                icon.name: "document-open"
                Material.foreground: window.buttonTextColor
                onClicked: ekgController.openFileDialog()
            }

            Button {
                text: "Zapisz wyniki"
                icon.name: "document-save"
                Material.foreground: window.buttonTextColor
            }

            //DARK / LIGHT
            Rectangle {
                id: themeToggle
                width: 52
                height: 28
                radius: height / 2
                // inne tło w dark / light
                color: isDarkTheme ? "#111827" : "#e5e7eb"
                border.color: isDarkTheme ? "#4b5563" : "#d1d5db"
                border.width: 1
                Layout.alignment: Qt.AlignVCenter

                Rectangle {
                    id: knob
                    width: parent.height - 6
                    height: parent.height - 6
                    radius: height / 2
                    x: isDarkTheme ? 3 : parent.width - width - 3
                    y: 3
                    color: "#ffffff"

                    Behavior on x {
                        NumberAnimation { duration: 160; easing.type: Easing.InOutQuad }
                    }

                    Label {
                        anchors.centerIn: parent
                        text: isDarkTheme ? "👨🏿" : "🔆"
                        font.pixelSize: 14
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: window.isDarkTheme = !window.isDarkTheme
                    cursorShape: Qt.PointingHandCursor
                }
            }





            Button {
                icon.name: "help-about"
                text: ""
                onClicked: helpDialog.open()
                Material.foreground: window.buttonTextColor
            }
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

                Label {
                    id: loadedFileLabel
                    text: ekgController.isFileLoaded ? "Załadowano: " + ekgController.loadedFilename : "Brak załadowanego pliku"
                    font.pixelSize: 12
                    color: textSecondary
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
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
                    color: borderColor
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

                    //poprawka dla comboboxa zeby czcionka pozostawala odpowiednia dla danego motywu
                    Material.foreground: window.isDarkTheme ? "#f9fafb" : "#111827"


                    delegate: ItemDelegate {
                        width: ListView.view.width
                        text: modelData
                        font: moduleCombo.font


                        Material.foreground: window.isDarkTheme ? "#f9fafb" : "#111827"
                    }
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
                    color: vizBg
                    border.color: vizBorder
                    border.width: 1

                    Label {
                        anchors.centerIn: parent
                        text: "Tutaj moduł Visualization wstawi wykresy EKG."
                        color: textSecondary
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
                    color: textSecondary
                    wrapMode: Text.WordWrap
                }

                Label {
                    visible: !ekgController.hasData && window.currentModule === "ECG BASELINE"
                    text: "⚠️ Najpierw zaimportuj sygnał EKG"
                    color: Material.color(Material.Orange)
                    font.bold: true
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Loader {
                    id: paramsLoader
                    Layout.fillWidth: true
                    sourceComponent:
                        window.currentModule === "ECG BASELINE" ? baselineParams :
                        window.currentModule === "R PEAKS"      ? rPeaksParams :
                        null
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        id: analysisStatus
                        text: ekgController.hasData ? "Status: oczekiwanie na analizę" : "Status: oczekiwanie na import pliku"
                        color: textSecondary
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                    }

                    ProgressBar {
                        id: analysisProgress
                        from: 0
                        to: 100
                        value: 0
                        Layout.fillWidth: true
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
                        enabled: ekgController.hasData
                        
                        ToolTip.visible: !ekgController.hasData && hovered
                        ToolTip.text: "Najpierw zaimportuj plik sygnału EKG"
                        ToolTip.delay: 500
                        
                        onClicked: {
                            if (window.currentModule === "ECG BASELINE") {
                                if (paramsLoader.item && paramsLoader.item.runFiltering) {
                                    paramsLoader.item.runFiltering()
                                }
                            } else {
                                analysisStatus.text = "Status: analiza w toku (demo)..."
                                analysisProgress.value = 0
                                fakeProgress.restart()
                            }
                        }
                    }

                    Button {
                        text: "Reset"
                        Layout.preferredWidth: 100
                        onClicked: {
                            fakeProgress.stop()
                            analysisProgress.value = 0
                            analysisStatus.text = "Status: oczekiwanie na analizę"

                            if (paramsLoader.item && paramsLoader.item.resetState) {
                                paramsLoader.item.resetState()
                            }
                        }
                    }
                }
            }
        }
    }

    //Timer – demo postępu analizy
    Timer {
        id: fakeProgress
        interval: 60
        repeat: true
        running: false
        onTriggered: {
            if (analysisProgress.value < 100) {
                analysisProgress.value += 5
            } else {
                stop()
                analysisStatus.text = "Status: analiza zakończona (demo)"
            }
        }
    }

    //ECG BASELINE – parametry filtru
    Component {
        id: baselineParams

        Rectangle {
            id: baselineRoot
            Layout.fillWidth: true
            radius: 10
            color: panelColor
            border.color: borderColor
            border.width: 1
            clip: true
            implicitHeight: baselineColumn.implicitHeight + 20

            function resetState() {
                filterGroup.checkedButton = null
                polyDegreeSpin.value = 3
                showOnlyFiltered.checked = false
            }

            function runFiltering() {
                if (!filterGroup.checkedButton) {
                    analysisStatus.text = "Status: wybierz filtr"
                    analysisStatus.color = Material.color(Material.Orange)
                    return
                }

                analysisStatus.text = "Status: filtrowanie w toku..."
                analysisStatus.color = textSecondary
                analysisProgress.value = 0
                fakeProgress.restart()

                if (rbMovingAverage.checked) {
                    ekgController.runBaseline(0)
                } else if (rbButterworth.checked) {
                    ekgController.runBaseline(1)
                } else if (rbSavitzky.checked) {
                    ekgController.runBaseline(2)
                }
            }

            ColumnLayout {
                id: baselineColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Label {
                    text: "ECG BASELINE – ustawienia filtru"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

                Label {
                    text: "Wybierz maksymalnie dwa filtry do porównania:"
                    color: textSecondary
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }

                ButtonGroup {
                    id: filterGroup
                }

                RadioButton {
                    id: rbMovingAverage
                    text: "Moving Average"
                    ButtonGroup.group: filterGroup
                }

                RadioButton {
                    id: rbButterworth
                    text: "Butterworth"
                    ButtonGroup.group: filterGroup
                }

                RadioButton {
                    id: rbSavitzky
                    text: "Savitzky-Golay"
                    ButtonGroup.group: filterGroup
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: borderColor
                }

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

    //R PEAKS – wybór metod detekcji
    Component {
        id: rPeaksParams

        Rectangle {
            id: rPeaksRoot
            Layout.fillWidth: true
            radius: 10
            color: panelColor
            border.color: borderColor
            border.width: 1
            clip: true
            implicitHeight: rPeaksColumn.implicitHeight + 20

            function resetState() {
                rPeaksColumn.selectedMethods = 0
                cbPanTompkins.checked = false
                cbHilbert.checked = false
                cbWavelet.checked = false
            }

            ColumnLayout {
                id: rPeaksColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                property int selectedMethods: 0

                function handleToggle(cb) {
                    if (cb.checked) {
                        selectedMethods++
                    } else {
                        selectedMethods--
                    }
                }

                Label {
                    text: "R PEAKS – metoda detekcji"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

                Label {
                    text: "Wybierz metodę / metody detekcji pików R:"
                    color: textSecondary
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }

                CheckBox {
                    id: cbPanTompkins
                    text: "Pan-Tompkins"
                    onToggled: rPeaksColumn.handleToggle(this)
                }

                CheckBox {
                    id: cbHilbert
                    text: "Transformata Hilberta"
                    onToggled: rPeaksColumn.handleToggle(this)
                }

                CheckBox {
                    id: cbWavelet
                    text: "Falkowa (Wavelet)"
                    onToggled: rPeaksColumn.handleToggle(this)
                }
            }
        }
    }

    //OKNO POMOCY
    Dialog {
        id: helpDialog
        title: "Jak korzystać z EKG Analyzer"
        modal: true
        standardButtons: Dialog.Ok
        implicitWidth: 420

        onVisibleChanged: if (visible) {
            x = (window.width  - implicitWidth)  / 2
            y = (window.height - implicitHeight) / 2
        }

        contentItem: ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 8

            Label {
                leftPadding: 8
                text: "1. Wybierz plik EKG (Import sygnału).\n" +
                      "2. Wybierz moduł analizy (np. ECG BASELINE, R PEAKS).\n" +
                      "3. Ustaw parametry w prawym panelu.\n" +
                      "4. Kliknij „Uruchom analizę”, aby przetworzyć sygnał.\n\n"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

}
