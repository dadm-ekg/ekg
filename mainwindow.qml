import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtCharts

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
    Component.onCompleted: rebuildChannelOptions()

    property var chartRawSeries: []
    property var chartFilteredSeries: []
    property var chartRPeaksSeries: []
    property int selectedChannelIndex: 0
    property int maxPlottedPoints: 4000
    property var channelOptions: []

    function clampChannelIndex(idx) {
        if (channelOptions.length === 0)
            return 0
        return Math.min(Math.max(idx, 0), channelOptions.length - 1)
    }

    function rebuildChannelOptions() {
        var count = ekgController.channelCount()
        var items = []
        for (var i = 0; i < count; ++i) {
            items.push("Kanał " + (i + 1))
        }
        channelOptions = items
        selectedChannelIndex = clampChannelIndex(selectedChannelIndex)
    }

    function refreshVisualization() {
        if (!ekgController.hasData) {
            chartRawSeries = []
            chartFilteredSeries = []
            chartRPeaksSeries = []
            Qt.callLater(applySeriesToChart)
            return
        }

        var channel = clampChannelIndex(selectedChannelIndex)
        chartRawSeries = ekgController.getRawSeries(channel, maxPlottedPoints)
        chartFilteredSeries = ekgController.getFilteredSeries(channel, maxPlottedPoints)
        chartRPeaksSeries = ekgController.getRPeakMarkers(channel)
        Qt.callLater(applySeriesToChart)
    }

    function applySeriesToChart() {
        if (typeof rawSeriesLine === "undefined" || typeof filteredSeriesLine === "undefined" || typeof peaksSeries === "undefined")
            return

        function updateSeries(series, data) {
            series.clear()
            var loggedSample = false
            for (var i = 0; i < data.length; ++i) {
                var p = data[i]
                var xVal = undefined
                var yVal = undefined
                if (p !== undefined && p !== null) {
                    if (p.x !== undefined) {
                        xVal = Number(p.x)
                        yVal = Number(p.y)
                    } else if (p["x"] !== undefined) {
                        xVal = Number(p["x"])
                        yVal = Number(p["y"])
                    }
                }
                if (xVal === undefined || yVal === undefined || isNaN(xVal) || isNaN(yVal))
                    continue
                series.append(xVal, yVal)
                if (!loggedSample && i === 0) {
                    console.log("QML append first point", xVal, yVal)
                    loggedSample = true
                }
            }
            series.visible = data.length > 0
        }

        console.log("QML chart update - raw:", chartRawSeries.length, "filtered:", chartFilteredSeries.length, "peaks:", chartRPeaksSeries.length, "channel:", selectedChannelIndex)

        updateSeries(rawSeriesLine, chartRawSeries)
        updateSeries(filteredSeriesLine, chartFilteredSeries)
        updateSeries(peaksSeries, chartRPeaksSeries)
        rescaleChart()
    }

    function rescaleChart() {
        if (!ekgController.hasData || typeof chartAxisX === "undefined" || typeof chartAxisY === "undefined")
            return

        var minY = Number.MAX_VALUE
        var maxY = -Number.MAX_VALUE
        var maxX = 0

        function scan(series) {
            for (var i = 0; i < series.length; ++i) {
                var p = series[i]
                if (!p) continue
                if (p.y < minY) minY = p.y
                if (p.y > maxY) maxY = p.y
                if (p.x > maxX) maxX = p.x
            }
        }

        scan(chartRawSeries)
        scan(chartFilteredSeries)
        scan(chartRPeaksSeries)

        if (minY === Number.MAX_VALUE || maxY === -Number.MAX_VALUE) {
            minY = -1
            maxY = 1
        } else if (minY === maxY) {
            minY -= 0.5
            maxY += 0.5
        }

        chartAxisX.min = 0
        chartAxisX.max = maxX > 0 ? maxX : 1
        chartAxisY.min = minY
        chartAxisY.max = maxY
    }

    function chartDurationSec() {
        if (chartFilteredSeries.length < 2)
            return 0
        var firstX = Number(chartFilteredSeries[0].x !== undefined ? chartFilteredSeries[0].x : chartFilteredSeries[0]["x"])
        var lastX = Number(chartFilteredSeries[chartFilteredSeries.length - 1].x !== undefined ? chartFilteredSeries[chartFilteredSeries.length - 1].x : chartFilteredSeries[chartFilteredSeries.length - 1]["x"])
        if (isNaN(firstX) || isNaN(lastX))
            return 0
        return Math.max(0, lastX - firstX)
    }

    function heartRateEstimate() {
        var duration = chartDurationSec()
        if (duration <= 0 || chartRPeaksSeries.length < 2)
            return 0
        return (chartRPeaksSeries.length / duration) * 60.0
    }

    property string currentModule: "ECG BASELINE"
    onCurrentModuleChanged: {
        analysisStatus.isProcessing = false
        fakeProgress.stop()
        analysisProgress.value = 0
        refreshVisualization()
    }

    Connections {
        target: ekgController
        function onFileLoadSuccess(filename) {
            analysisProgress.value = 0
            rebuildChannelOptions()
            refreshVisualization()
        }
        function onFileLoadError(errorMessage) {
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }
        function onFilteringSuccess(filterName) {
            console.log("Filtering success signal received")
            analysisStatus.isProcessing = false
            fakeProgress.stop()
            analysisProgress.value = 100
            refreshVisualization()
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
            refreshVisualization()
        }
        function onBaselineCompletedChanged() {
            if (ekgController.baselineCompleted) {
                console.log("Baseline completed changed to true")
                analysisStatus.isProcessing = false
                fakeProgress.stop()
                analysisProgress.value = 100
                refreshVisualization()
            }
        }
        function onRPeaksCompletedChanged() {
            if (ekgController.rPeaksCompleted) {
                console.log("R peaks completed changed to true")
                analysisStatus.isProcessing = false
                fakeProgress.stop()
                analysisProgress.value = 100
                refreshVisualization()
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
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            ComboBox {
                                id: channelCombo
                                model: channelOptions
                                Layout.preferredWidth: 150
                                enabled: model.length > 0
                                currentIndex: selectedChannelIndex
                                onActivated: {
                                    selectedChannelIndex = currentIndex
                                    refreshVisualization()
                                }
                            }

                            Button {
                                text: "Odśwież"
                                onClicked: refreshVisualization()
                            }

                            Button {
                                text: "Eksportuj PNG"
                                enabled: ekgController.hasData
                                onClicked: exportDialog.open()
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            visible: window.currentModule === "R PEAKS"

                            Label {
                                text: "Piki R: " + chartRPeaksSeries.length
                                color: textSecondary
                            }

                            Label {
                                text: {
                                    var bpm = heartRateEstimate()
                                    return "HR (bpm): " + (bpm > 0 ? Math.round(bpm) : "-")
                                }
                                color: textSecondary
                            }

                            Label {
                                text: {
                                    var dur = chartDurationSec()
                                    return "Czas okna: " + (dur > 0 ? dur.toFixed(1) + " s" : "-")
                                }
                                color: textSecondary
                            }
                        }

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            ChartView {
                                id: signalChart
                                anchors.fill: parent
                                antialiasing: true
                                theme: isDarkTheme ? ChartView.ChartThemeDark : ChartView.ChartThemeLight
                                backgroundColor: vizBg
                                legend.visible: true
                                legend.alignment: Qt.AlignTop
                                legend.labelColor: textSecondary
                                enabled: ekgController.hasData

                                ValueAxis {
                                    id: chartAxisX
                                    titleText: "Czas [s]"
                                    labelFormat: "%.2f"
                                    labelsColor: textSecondary
                                    gridVisible: false
                                }

                                ValueAxis {
                                    id: chartAxisY
                                    titleText: "Amplituda [mV]"
                                    labelsColor: textSecondary
                                }

                                LineSeries {
                                    id: rawSeriesLine
                                    name: "Sygnał surowy"
                                    color: "#60a5fa"
                                    axisX: chartAxisX
                                    axisY: chartAxisY
                                    visible: chartRawSeries.length > 0
                                }

                                LineSeries {
                                    id: filteredSeriesLine
                                    name: "Sygnał filtrowany"
                                    color: "#10b981"
                                    axisX: chartAxisX
                                    axisY: chartAxisY
                                    visible: chartFilteredSeries.length > 0
                                }

                                ScatterSeries {
                                    id: peaksSeries
                                    name: "Piki R"
                                    color: "#ef4444"
                                    markerShape: ScatterSeries.MarkerShapeCircle
                                    markerSize: 8
                                    axisX: chartAxisX
                                    axisY: chartAxisY
                                    visible: chartRPeaksSeries.length > 0 && window.currentModule === "R PEAKS"
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                visible: !ekgController.hasData || (chartRawSeries.length === 0 && chartFilteredSeries.length === 0)
                                text: "Załaduj plik EKG, aby zobaczyć wizualizację."
                                color: textSecondary
                            }
                        }
                    }

                    FileDialog {
                        id: exportDialog
                        title: "Zapisz wykres do pliku"
                        fileMode: FileDialog.SaveFile
                        nameFilters: ["PNG (*.png)"]
                        onAccepted: {
                            if (selectedFile && signalChart) {
                                var target = selectedFile.toString()
                                if (!target.toLowerCase().endsWith(".png"))
                                    target = target + ".png"
                                signalChart.grabToImage(function(result) {
                                    result.saveToFile(target)
                                })
                            }
                        }
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
                            ekgController.resetBaseline()
                            chartRawSeries = ekgController.getRawSeries(selectedChannelIndex, maxPlottedPoints)
                            chartFilteredSeries = []
                            chartRPeaksSeries = []
                            applySeriesToChart()

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
