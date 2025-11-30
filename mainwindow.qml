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
    onCurrentModuleChanged: {
        analysisStatus.isProcessing = false
        fakeProgress.stop()
        analysisProgress.value = 0
    }

    Connections {
        target: ekgController
        function onFileLoadSuccess(filename) {
            analysisProgress.value = 0
        }
        function onFileLoadError(errorMessage) {
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }
        function onFilteringSuccess(filterName) {
            console.log("Filtering success signal received")
            analysisStatus.isProcessing = false
            fakeProgress.stop()
            analysisProgress.value = 100
        }
        function onFilteringError(errorMessage) {
            analysisStatus.isProcessing = false
            fakeProgress.stop()
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }
        function onRPeaksDetectionSuccess(methodName) {
            console.log("R peaks detection success signal received")
            analysisStatus.isProcessing = false
            fakeProgress.stop()
            analysisProgress.value = 100
        }
        function onBaselineCompletedChanged() {
            if (ekgController.baselineCompleted) {
                console.log("Baseline completed changed to true")
                analysisStatus.isProcessing = false
                fakeProgress.stop()
                analysisProgress.value = 100
            }
        }
        function onRPeaksCompletedChanged() {
            if (ekgController.rPeaksCompleted) {
                console.log("R peaks completed changed to true")
                analysisStatus.isProcessing = false
                fakeProgress.stop()
                analysisProgress.value = 100
            }
        }
        function onRPeaksDetectionError(errorMessage) {
            analysisStatus.isProcessing = false
            fakeProgress.stop()
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }
    }

    function showTemporaryStatus(message, color) {
        tempStatusText = analysisStatus.text
        tempStatusColor = analysisStatus.color
        analysisStatus.text = message
        analysisStatus.color = Material.color(color)
        statusResetTimer.restart()
    }

    property string tempStatusText: ""
    property color tempStatusColor: textSecondary

    Timer {
        id: statusResetTimer
        interval: 3000
        repeat: false
        onTriggered: {
            analysisStatus.text = tempStatusText
            analysisStatus.color = tempStatusColor
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
                    text: ekgController.hasData ? "✓ Załadowano: " + ekgController.loadedFilename : "Brak załadowanego pliku"
                    font.pixelSize: 12
                    color: ekgController.hasData ? Material.color(Material.Green) : textSecondary
                    font.bold: ekgController.hasData
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                TextField {
                    id: fileFilterField
                    placeholderText: "Filtruj listę..."
                    Layout.fillWidth: true
                }

                ListView {
                    id: fileList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    
                    model: {
                        var allFiles = ekgController.getAvailableFiles()
                        if (fileFilterField.text === "") {
                            return allFiles
                        }
                        var filtered = []
                        for (var i = 0; i < allFiles.length; i++) {
                            if (allFiles[i].toLowerCase().indexOf(fileFilterField.text.toLowerCase()) !== -1) {
                                filtered.push(allFiles[i])
                            }
                        }
                        return filtered
                    }
                    
                    delegate: ItemDelegate {
                        width: ListView.view.width
                        text: modelData
                        highlighted: ekgController.loadedFilename === modelData
                        
                        onClicked: {
                            ekgController.loadFileByName(modelData)
                        }
                    }
                    
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
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

                Label {
                    visible: !ekgController.hasFilteredData && window.currentModule === "R PEAKS"
                    text: "⚠️ Najpierw uruchom filtrowanie baseline (moduł ECG BASELINE)"
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
                        property bool isProcessing: false
                        property string processingText: ""
                        property string statusText: {
                            var module = window.currentModule
                            var hasData = ekgController.hasData
                            var hasFiltered = ekgController.hasFilteredData
                            var baselineOK = ekgController.baselineCompleted
                            var rPeaksOK = ekgController.rPeaksCompleted
                            
                            console.log("Status update - module:", module, "hasData:", hasData, "baselineCompleted:", baselineOK, "rPeaksCompleted:", rPeaksOK)
                            
                            if (module === "ECG BASELINE") {
                                if (!hasData) {
                                    return "Oczekiwanie na plik"
                                } else if (!baselineOK) {
                                    return "Gotowy"
                                } else {
                                    return "Skończono"
                                }
                            } else if (module === "R PEAKS") {
                                if (!hasData) {
                                    return "Oczekiwanie na plik"
                                } else if (!hasFiltered) {
                                    return "Oczekiwanie na filtrowanie"
                                } else if (!rPeaksOK) {
                                    return "Gotowy"
                                } else {
                                    return "Skończono"
                                }
                            } else {
                                return hasData ? "Oczekiwanie na analizę" : "Oczekiwanie na import"
                            }
                        }
                        text: isProcessing ? processingText : statusText
                        property color statusColor: {
                            var module = window.currentModule
                            var hasData = ekgController.hasData
                            var hasFiltered = ekgController.hasFilteredData
                            var baselineOK = ekgController.baselineCompleted
                            var rPeaksOK = ekgController.rPeaksCompleted
                            
                            if (module === "ECG BASELINE") {
                                if (!hasData) return textSecondary
                                if (!baselineOK) return Material.color(Material.Teal)
                                return Material.color(Material.Green)
                            } else if (module === "R PEAKS") {
                                if (!hasData) return textSecondary
                                if (!hasFiltered) return textSecondary
                                if (!rPeaksOK) return Material.color(Material.Teal)
                                return Material.color(Material.Green)
                            }
                            return textSecondary
                        }
                        color: statusColor
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Component.onCompleted: {
                            Qt.callLater(function() { })
                        }
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
                        enabled: {
                            if (window.currentModule === "ECG BASELINE") {
                                return ekgController.hasData
                            } else if (window.currentModule === "R PEAKS") {
                                return ekgController.hasFilteredData
                            }
                            return true
                        }
                        
                        ToolTip.visible: hovered && !enabled
                        ToolTip.text: {
                            if (window.currentModule === "ECG BASELINE" && !ekgController.hasData) {
                                return "Najpierw zaimportuj plik sygnału EKG"
                            } else if (window.currentModule === "R PEAKS" && !ekgController.hasFilteredData) {
                                return "Najpierw uruchom filtrowanie baseline"
                            }
                            return ""
                        }
                        ToolTip.delay: 500
                        
                        onClicked: {
                            if (window.currentModule === "ECG BASELINE") {
                                if (paramsLoader.item && paramsLoader.item.runFiltering) {
                                    paramsLoader.item.runFiltering()
                                }
                            } else if (window.currentModule === "R PEAKS") {
                                if (paramsLoader.item && paramsLoader.item.runDetection) {
                                    paramsLoader.item.runDetection()
                                }
                            } else {
                                analysisStatus.text = "Analiza w toku (demo)..."
                                analysisStatus.color = textSecondary
                                analysisProgress.value = 0
                                fakeProgress.restart()
                            }
                        }
                    }

                    Button {
                        text: "Reset"
                        Layout.preferredWidth: 100
                        onClicked: {
                            analysisStatus.isProcessing = false
                            fakeProgress.stop()
                            statusResetTimer.stop()
                            analysisProgress.value = 0

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
                analysisStatus.text = "✓ Analiza zakończona (demo)"
                analysisStatus.color = Material.color(Material.Green)
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
                    showTemporaryStatus("⚠ Wybierz filtr", Material.Orange)
                    return
                }

                analysisStatus.isProcessing = true
                analysisStatus.processingText = "Filtrowanie w toku..."
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
                detectionMethodGroup.checkedButton = null
            }

            function runDetection() {
                if (!detectionMethodGroup.checkedButton) {
                    showTemporaryStatus("⚠ Wybierz metodę detekcji", Material.Orange)
                    return
                }

                analysisStatus.isProcessing = true
                analysisStatus.processingText = "Detekcja pików R w toku..."
                analysisProgress.value = 0
                fakeProgress.restart()

                if (rbPanTompkins.checked) {
                    ekgController.runRPeaksDetection(0)
                } else if (rbHilbert.checked) {
                    ekgController.runRPeaksDetection(1)
                } else if (rbWavelet.checked) {
                    ekgController.runRPeaksDetection(2)
                }
            }

            ColumnLayout {
                id: rPeaksColumn
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Label {
                    text: "R PEAKS – metoda detekcji"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

                Label {
                    text: "Wybierz metodę detekcji pików R:"
                    color: textSecondary
                    wrapMode: Text.WordWrap
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }

                ButtonGroup {
                    id: detectionMethodGroup
                }

                RadioButton {
                    id: rbPanTompkins
                    text: "Pan-Tompkins"
                    ButtonGroup.group: detectionMethodGroup
                }

                RadioButton {
                    id: rbHilbert
                    text: "Transformata Hilberta"
                    ButtonGroup.group: detectionMethodGroup
                }

                RadioButton {
                    id: rbWavelet
                    text: "Falkowa (Wavelet)"
                    ButtonGroup.group: detectionMethodGroup
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
