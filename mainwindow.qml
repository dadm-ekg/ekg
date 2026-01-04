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

    property bool isDarkTheme: true

    property color bgMain:        isDarkTheme ? "#020712" : "#f3f4f6"
    property color panelColor:    isDarkTheme ? "#050d18" : "#ffffff"
    property color borderColor:   isDarkTheme ? "#1f2933" : "#d1d5db"
    property color textSecondary: isDarkTheme ? "#9ca3af" : "#4b5563"
    property color vizBg:         isDarkTheme ? "#020812" : "#f9fafb"
    property color vizBorder:     isDarkTheme ? "#0f172a" : "#d1d5db"

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
    property bool chartLoading: false
    property string pendingFileName: ""
    property string lastUsedFilter: ""
    property string lastUsedRPeaksMethod: ""
    property string lastUsedHRVTimeMethod: ""
    property bool hrvGeoRan: false
    property bool wavesRan: false
    property int selectedFilterMethod: -1
    property int selectedRPeaksMethod: -1
    property int selectedHRVTimeMethod: -1
    property var chartWaveMarkers: {}

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
            chartLoading = false
            Qt.callLater(applySeriesToChart)
            return
        }

        if (!chartLoading) {
            chartLoading = true
        }
        Qt.callLater(function() {
            var channel = clampChannelIndex(selectedChannelIndex)
            chartRawSeries = ekgController.getRawSeries(channel, maxPlottedPoints)
            chartFilteredSeries = ekgController.getFilteredSeries(channel, maxPlottedPoints)
            chartRPeaksSeries = ekgController.getRPeakMarkers(channel)
            chartWaveMarkers = ekgController.getWaveMarkers(channel)
            Qt.callLater(applySeriesToChart)
        })
    }

    function scheduleVisualizationRefresh() {
        vizDebounce.restart()
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
        
        if (chartWaveMarkers) {
            updateSeries(pOnsetSeries, chartWaveMarkers.p_onsets || [])
            updateSeries(pEndSeries, chartWaveMarkers.p_ends || [])
            updateSeries(qrsOnsetSeries, chartWaveMarkers.qrs_onsets || [])
            updateSeries(qrsEndSeries, chartWaveMarkers.qrs_ends || [])
            updateSeries(tEndSeries, chartWaveMarkers.t_ends || [])
        }
        
        updateMarkerVisibility()
        
        rescaleChart()
        chartLoading = false
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

    property string currentModule: "ECG BASELINE"
    onCurrentModuleChanged: {
        analysisStatus.isProcessing = false
        analysisProgress.value = 0
        updateMarkerVisibility()
    }

    function updateMarkerVisibility() {
        peaksSeries.visible = window.currentModule === "R PEAKS" && chartRPeaksSeries.length > 0
        
        var showWaves = window.currentModule === "WAVES" && ekgController.wavesCompleted
        pOnsetSeries.visible = showWaves
        pEndSeries.visible = showWaves
        qrsOnsetSeries.visible = showWaves
        qrsEndSeries.visible = showWaves
        tEndSeries.visible = showWaves
    }

    Connections {
        target: ekgController
        function onFileLoadSuccess(filename) {
            analysisProgress.value = 0
            lastUsedFilter = ""
            lastUsedRPeaksMethod = ""
            lastUsedHRVTimeMethod = ""
            selectedFilterMethod = -1
            selectedRPeaksMethod = -1
            selectedHRVTimeMethod = -1
            hrvGeoRan = false
            wavesRan = false
            chartWaveMarkers = {}
            rebuildChannelOptions()
            refreshVisualization()
        }
        function onFileLoadError(errorMessage) {
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }
        function onFilteringSuccess(filterName) {
            lastUsedFilter = filterName
            analysisStatus.isProcessing = false
            analysisProgress.value = 100
            refreshVisualization()
        }
        function onFilteringError(errorMessage) {
            analysisStatus.isProcessing = false
            chartLoading = false
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }
        function onRPeaksDetectionSuccess(methodName) {
            lastUsedRPeaksMethod = methodName
            analysisStatus.isProcessing = false
            analysisProgress.value = 100
            refreshVisualization()
        }
        function onBaselineCompletedChanged() {
            if (ekgController.baselineCompleted) {
                analysisStatus.isProcessing = false
                analysisProgress.value = 100
                refreshVisualization()
            }
        }
        function onRPeaksCompletedChanged() {
            if (ekgController.rPeaksCompleted) {
                analysisStatus.isProcessing = false
                analysisProgress.value = 100
                refreshVisualization()
            }
        }
        function onRPeaksDetectionError(errorMessage) {
            analysisStatus.isProcessing = false
            chartLoading = false
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }
        function onHrvTimeSuccess(methodName) {
            lastUsedHRVTimeMethod = methodName
            analysisStatus.isProcessing = false
            analysisProgress.value = 100
        }
        function onHrvTimeError(errorMessage) {
            analysisStatus.isProcessing = false
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }
        function onHrvTimeCompletedChanged() {
            if (ekgController.hrvTimeCompleted) {
                analysisStatus.isProcessing = false
                analysisProgress.value = 100
            }
        }
        function onHrvGeoSuccess() {
            hrvGeoRan = true
            analysisStatus.isProcessing = false
            analysisProgress.value = 100
        }
        function onHrvGeoError(errorMessage) {
            analysisStatus.isProcessing = false
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }
        function onHrvGeoCompletedChanged() {
            if (ekgController.hrvGeoCompleted) {
                analysisStatus.isProcessing = false
                analysisProgress.value = 100
            }
        }
        function onWavesSuccess() {
            wavesRan = true
            analysisStatus.isProcessing = false
            analysisProgress.value = 100
            refreshVisualization()
        }
        function onWavesError(errorMessage) {
            analysisStatus.isProcessing = false
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }
        function onWavesCompletedChanged() {
            if (ekgController.wavesCompleted) {
                analysisStatus.isProcessing = false
                analysisProgress.value = 100
                refreshVisualization()
            }
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

    Timer {
        id: vizDebounce
        interval: 150
        repeat: false
        onTriggered: refreshVisualization()
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

            Rectangle {
                id: themeToggle
                width: 52
                height: 28
                radius: height / 2
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
                            pendingFileName = modelData
                            loadConfirmDialog.open()
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
                        "HRV TIME",
                        "HRV GEO"
                    ]
                    onCurrentTextChanged: window.currentModule = currentText

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
                                enabled: model.length > 0 && !chartLoading
                                currentIndex: selectedChannelIndex
                                onActivated: {
                                    selectedChannelIndex = currentIndex
                                    chartLoading = true
                                    scheduleVisualizationRefresh()
                                }
                            }

                            Button {
                                text: "Eksportuj PNG"
                                enabled: ekgController.hasData
                                onClicked: exportDialog.open()
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

                                ScatterSeries {
                                    id: pOnsetSeries
                                    name: "P onset"
                                    color: "#f59e0b"
                                    markerShape: ScatterSeries.MarkerShapeCircle
                                    markerSize: 6
                                    axisX: chartAxisX
                                    axisY: chartAxisY
                                    visible: window.currentModule === "WAVES" && ekgController.wavesCompleted
                                }

                                ScatterSeries {
                                    id: pEndSeries
                                    name: "P end"
                                    color: "#eab308"
                                    markerShape: ScatterSeries.MarkerShapeCircle
                                    markerSize: 6
                                    axisX: chartAxisX
                                    axisY: chartAxisY
                                    visible: window.currentModule === "WAVES" && ekgController.wavesCompleted
                                }

                                ScatterSeries {
                                    id: qrsOnsetSeries
                                    name: "QRS onset"
                                    color: "#8b5cf6"
                                    markerShape: ScatterSeries.MarkerShapeCircle
                                    markerSize: 6
                                    axisX: chartAxisX
                                    axisY: chartAxisY
                                    visible: window.currentModule === "WAVES" && ekgController.wavesCompleted
                                }

                                ScatterSeries {
                                    id: qrsEndSeries
                                    name: "QRS end"
                                    color: "#a78bfa"
                                    markerShape: ScatterSeries.MarkerShapeCircle
                                    markerSize: 6
                                    axisX: chartAxisX
                                    axisY: chartAxisY
                                    visible: window.currentModule === "WAVES" && ekgController.wavesCompleted
                                }

                                ScatterSeries {
                                    id: tEndSeries
                                    name: "T end"
                                    color: "#ec4899"
                                    markerShape: ScatterSeries.MarkerShapeCircle
                                    markerSize: 6
                                    axisX: chartAxisX
                                    axisY: chartAxisY
                                    visible: window.currentModule === "WAVES" && ekgController.wavesCompleted
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                visible: !ekgController.hasData || (chartRawSeries.length === 0 && chartFilteredSeries.length === 0)
                                text: "Załaduj plik EKG, aby zobaczyć wizualizację."
                                color: textSecondary
                            }

                            Rectangle {
                                anchors.fill: parent
                                color: isDarkTheme ? Qt.rgba(0, 0, 0, 0.5) : Qt.rgba(1, 1, 1, 0.5)
                                visible: chartLoading
                                z: 2

                                BusyIndicator {
                                    anchors.centerIn: parent
                                    running: chartLoading
                                    width: 64
                                    height: 64
                                }
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
                    text: window.currentModule
                    font.bold: true
                    font.pixelSize: 18
                }

                Label {
                    visible: !ekgController.hasData && window.currentModule === "ECG BASELINE"
                    text: "⚠️ Najpierw zaimportuj sygnal EKG"
                    color: Material.color(Material.Orange)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Label {
                    visible: !ekgController.hasFilteredData && window.currentModule === "R PEAKS"
                    text: "⚠️ Najpierw uruchom filtrowanie baseline"
                    color: Material.color(Material.Orange)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Label {
                    visible: !ekgController.rPeaksCompleted && window.currentModule === "HRV TIME"
                    text: "⚠️ Najpierw uruchom detekcje pikow R"
                    color: Material.color(Material.Orange)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Label {
                    visible: !ekgController.rPeaksCompleted && window.currentModule === "HRV GEO"
                    text: "⚠️ Najpierw uruchom detekcje pikow R"
                    color: Material.color(Material.Orange)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Label {
                    visible: !ekgController.hasFilteredData && window.currentModule === "WAVES"
                    text: "⚠️ Najpierw uruchom filtrowanie baseline"
                    color: Material.color(Material.Orange)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        id: analysisStatus
                        property bool isProcessing: false
                        property string statusText: {
                            var module = window.currentModule
                            var hasData = ekgController.hasData
                            var hasFiltered = ekgController.hasFilteredData
                            var baselineOK = ekgController.baselineCompleted
                            var rPeaksOK = ekgController.rPeaksCompleted
                            var hrvTimeOK = ekgController.hrvTimeCompleted
                            
                            if (module === "ECG BASELINE") {
                                if (!hasData) {
                                    return "Oczekiwanie na plik"
                                } else if (isProcessing) {
                                    return "Przetwarzanie..."
                                } else if (!baselineOK) {
                                    return "Gotowy"
                                } else {
                                    if (lastUsedFilter !== "") {
                                        return "Przefiltrowano z uzyciem " + lastUsedFilter
                                    } else {
                                        return "Skonczono"
                                    }
                                }
                            } else if (module === "R PEAKS") {
                                if (!hasData) {
                                    return "Oczekiwanie na plik"
                                } else if (!hasFiltered) {
                                    return "Oczekiwanie na filtrowanie"
                                } else if (isProcessing) {
                                    return "Przetwarzanie..."
                                } else if (!rPeaksOK) {
                                    return "Gotowy"
                                } else {
                                    if (lastUsedRPeaksMethod !== "") {
                                        return "Wykryto piki R metoda " + lastUsedRPeaksMethod
                                    } else {
                                        return "Skonczono"
                                    }
                                }
                            } else if (module === "HRV TIME") {
                                if (!hasData) {
                                    return "Oczekiwanie na plik"
                                } else if (!rPeaksOK) {
                                    return "Oczekiwanie na detekcje R"
                                } else if (isProcessing) {
                                    return "Przetwarzanie..."
                                } else if (!hrvTimeOK) {
                                    return "Gotowy"
                                } else {
                                    if (lastUsedHRVTimeMethod !== "") {
                                        return "Obliczono metoda " + lastUsedHRVTimeMethod
                                    } else {
                                        return "Skonczono"
                                    }
                                }
                            } else if (module === "HRV GEO") {
                                var hrvGeoOK = ekgController.hrvGeoCompleted
                                if (!hasData) {
                                    return "Oczekiwanie na plik"
                                } else if (!rPeaksOK) {
                                    return "Oczekiwanie na detekcje R"
                                } else if (isProcessing) {
                                    return "Przetwarzanie..."
                                } else if (!hrvGeoOK) {
                                    return "Gotowy"
                                } else {
                                    return "Obliczono metryki geometryczne"
                                }
                            } else if (module === "WAVES") {
                                var wavesOK = ekgController.wavesCompleted
                                if (!hasData) {
                                    return "Oczekiwanie na plik"
                                } else if (!hasFiltered) {
                                    return "Oczekiwanie na filtrowanie"
                                } else if (isProcessing) {
                                    return "Przetwarzanie..."
                                } else if (!wavesOK) {
                                    return "Gotowy"
                                } else {
                                    return "Wykryto fale EKG"
                                }
                            } else {
                                return hasData ? "Oczekiwanie na analize" : "Oczekiwanie na import"
                            }
                        }
                        text: statusText
                        property color statusColor: {
                            var module = window.currentModule
                            var hasData = ekgController.hasData
                            var hasFiltered = ekgController.hasFilteredData
                            var baselineOK = ekgController.baselineCompleted
                            var rPeaksOK = ekgController.rPeaksCompleted
                            var hrvTimeOK = ekgController.hrvTimeCompleted
                            
                            if (module === "ECG BASELINE") {
                                if (!hasData) return textSecondary
                                if (isProcessing) return Material.color(Material.Orange)
                                if (!baselineOK) return Material.color(Material.Teal)
                                return Material.color(Material.Green)
                            } else if (module === "R PEAKS") {
                                if (!hasData) return textSecondary
                                if (!hasFiltered) return textSecondary
                                if (isProcessing) return Material.color(Material.Orange)
                                if (!rPeaksOK) return Material.color(Material.Teal)
                                return Material.color(Material.Green)
                            } else if (module === "HRV TIME") {
                                if (!hasData) return textSecondary
                                if (!rPeaksOK) return textSecondary
                                if (isProcessing) return Material.color(Material.Orange)
                                if (!hrvTimeOK) return Material.color(Material.Teal)
                                return Material.color(Material.Green)
                            } else if (module === "HRV GEO") {
                                var hrvGeoOK = ekgController.hrvGeoCompleted
                                if (!hasData) return textSecondary
                                if (!rPeaksOK) return textSecondary
                                if (isProcessing) return Material.color(Material.Orange)
                                if (!hrvGeoOK) return Material.color(Material.Teal)
                                return Material.color(Material.Green)
                            } else if (module === "WAVES") {
                                var wavesOK = ekgController.wavesCompleted
                                if (!hasData) return textSecondary
                                if (!hasFiltered) return textSecondary
                                if (isProcessing) return Material.color(Material.Orange)
                                if (!wavesOK) return Material.color(Material.Teal)
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

                Loader {
                    id: paramsLoader
                    Layout.fillWidth: true
                    sourceComponent:
                        window.currentModule === "ECG BASELINE" ? baselineParams :
                        window.currentModule === "R PEAKS"      ? rPeaksParams :
                        window.currentModule === "WAVES"        ? wavesParams :
                        window.currentModule === "HRV TIME"     ? hrvTimeParams :
                        window.currentModule === "HRV GEO"      ? hrvGeoParams :
                        null
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
                            var params = paramsLoader.item
                            if (window.currentModule === "ECG BASELINE") {
                                return params && params.isReady && params.isReady()
                            } else if (window.currentModule === "R PEAKS") {
                                return params && params.isReady && params.isReady()
                            } else if (window.currentModule === "HRV TIME") {
                                return params && params.isReady && params.isReady()
                            } else if (window.currentModule === "HRV GEO") {
                                return ekgController.rPeaksCompleted
                            } else if (window.currentModule === "WAVES") {
                                return ekgController.hasFilteredData
                            }
                            return true
                        }
                        
                        ToolTip.visible: hovered && !enabled
                        ToolTip.text: {
                            var params = paramsLoader.item
                            if (window.currentModule === "ECG BASELINE" && !ekgController.hasData) {
                                return "Najpierw zaimportuj plik sygnału EKG"
                            } else if (window.currentModule === "ECG BASELINE" && (!params || !params.isReady || !params.isReady())) {
                                return "Wybierz filtr baseline"
                            } else if (window.currentModule === "R PEAKS" && !ekgController.hasFilteredData) {
                                return "Najpierw uruchom filtrowanie baseline"
                            } else if (window.currentModule === "R PEAKS" && (!params || !params.isReady || !params.isReady())) {
                                return "Wybierz metodę detekcji R"
                            } else if (window.currentModule === "HRV TIME" && !ekgController.rPeaksCompleted) {
                                return "Najpierw uruchom detekcję pików R"
                            } else if (window.currentModule === "HRV TIME" && (!params || !params.isReady || !params.isReady())) {
                                return "Wybierz metodę estymacji widma"
                            } else if (window.currentModule === "HRV GEO" && !ekgController.rPeaksCompleted) {
                                return "Najpierw uruchom detekcję pików R"
                            } else if (window.currentModule === "WAVES" && !ekgController.hasFilteredData) {
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
                            } else if (window.currentModule === "HRV TIME") {
                                if (paramsLoader.item && paramsLoader.item.runHRVTime) {
                                    paramsLoader.item.runHRVTime()
                                }
                            } else if (window.currentModule === "HRV GEO") {
                                if (paramsLoader.item && paramsLoader.item.runHRVGeo) {
                                    paramsLoader.item.runHRVGeo()
                                }
                            } else if (window.currentModule === "WAVES") {
                                if (paramsLoader.item && paramsLoader.item.runWaves) {
                                    paramsLoader.item.runWaves()
                                }
                            }
                        }
                    }

                    Button {
                        text: "Reset"
                        Layout.preferredWidth: 100
                        onClicked: {
                            analysisStatus.isProcessing = false
                            statusResetTimer.stop()
                            analysisProgress.value = 0
                            if (window.currentModule === "ECG BASELINE") {
                                ekgController.resetBaseline()
                                lastUsedFilter = ""
                                lastUsedRPeaksMethod = ""
                                lastUsedHRVTimeMethod = ""
                                hrvGeoRan = false
                                wavesRan = false
                                selectedFilterMethod = -1
                                selectedRPeaksMethod = -1
                                selectedHRVTimeMethod = -1
                                chartFilteredSeries = []
                                chartRPeaksSeries = []
                                chartWaveMarkers = {}
                                filteredSeriesLine.clear()
                                peaksSeries.clear()
                                pOnsetSeries.clear()
                                pEndSeries.clear()
                                qrsOnsetSeries.clear()
                                qrsEndSeries.clear()
                                tEndSeries.clear()
                                filteredSeriesLine.visible = false
                                peaksSeries.visible = false
                                pOnsetSeries.visible = false
                                pEndSeries.visible = false
                                qrsOnsetSeries.visible = false
                                qrsEndSeries.visible = false
                                tEndSeries.visible = false
                            } else if (window.currentModule === "R PEAKS") {
                                ekgController.resetRPeaks()
                                lastUsedRPeaksMethod = ""
                                lastUsedHRVTimeMethod = ""
                                hrvGeoRan = false
                                wavesRan = false
                                selectedRPeaksMethod = -1
                                selectedHRVTimeMethod = -1
                                chartRPeaksSeries = []
                                peaksSeries.clear()
                                peaksSeries.visible = false
                            } else if (window.currentModule === "HRV TIME") {
                                ekgController.resetHRVTime()
                                lastUsedHRVTimeMethod = ""
                                selectedHRVTimeMethod = -1
                            } else if (window.currentModule === "HRV GEO") {
                                ekgController.resetHRVGeo()
                                hrvGeoRan = false
                            } else if (window.currentModule === "WAVES") {
                                ekgController.resetWaves()
                                wavesRan = false
                                chartWaveMarkers = {}
                                pOnsetSeries.clear()
                                pEndSeries.clear()
                                qrsOnsetSeries.clear()
                                qrsEndSeries.clear()
                                tEndSeries.clear()
                                pOnsetSeries.visible = false
                                pEndSeries.visible = false
                                qrsOnsetSeries.visible = false
                                qrsEndSeries.visible = false
                                tEndSeries.visible = false
                            } else {
                                chartRawSeries = []
                                chartFilteredSeries = []
                                chartRPeaksSeries = []
                                chartWaveMarkers = {}
                                rawSeriesLine.clear()
                                filteredSeriesLine.clear()
                                peaksSeries.clear()
                                pOnsetSeries.clear()
                                pEndSeries.clear()
                                qrsOnsetSeries.clear()
                                qrsEndSeries.clear()
                                tEndSeries.clear()
                                rawSeriesLine.visible = false
                                filteredSeriesLine.visible = false
                                peaksSeries.visible = false
                                pOnsetSeries.visible = false
                                pEndSeries.visible = false
                                qrsOnsetSeries.visible = false
                                qrsEndSeries.visible = false
                                tEndSeries.visible = false
                            }

                            if (paramsLoader.item && paramsLoader.item.resetState) {
                                paramsLoader.item.resetState()
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: baselineParams

        ColumnLayout {
            id: baselineRoot
            Layout.fillWidth: true
            spacing: 8

            Component.onCompleted: {
                if (window.selectedFilterMethod === 0) rbMovingAverage.checked = true
                else if (window.selectedFilterMethod === 1) rbButterworth.checked = true
                else if (window.selectedFilterMethod === 2) rbSavitzky.checked = true
            }

            function isReady() {
                return ekgController.hasData && filterGroup.checkedButton !== null
            }

            function resetState() {
                filterGroup.checkedButton = null
                window.selectedFilterMethod = -1
            }

            function runFiltering() {
                if (!filterGroup.checkedButton) {
                    showTemporaryStatus("⚠ Wybierz filtr", Material.Orange)
                    return
                }

                analysisStatus.isProcessing = true
                chartLoading = true
                analysisProgress.value = 0

                if (rbMovingAverage.checked) {
                    window.selectedFilterMethod = 0
                    ekgController.runBaseline(0)
                } else if (rbButterworth.checked) {
                    window.selectedFilterMethod = 1
                    ekgController.runBaseline(1)
                } else if (rbSavitzky.checked) {
                    window.selectedFilterMethod = 2
                    ekgController.runBaseline(2)
                }
            }

            Label {
                text: "Wybierz filtr:"
                color: textSecondary
                font.pixelSize: 13
            }

            ButtonGroup {
                id: filterGroup
            }

            RadioButton {
                id: rbMovingAverage
                text: "Moving Average"
                ButtonGroup.group: filterGroup
                onCheckedChanged: if (checked) window.selectedFilterMethod = 0
            }

            RadioButton {
                id: rbButterworth
                text: "Butterworth"
                ButtonGroup.group: filterGroup
                onCheckedChanged: if (checked) window.selectedFilterMethod = 1
            }

            RadioButton {
                id: rbSavitzky
                text: "Savitzky-Golay"
                ButtonGroup.group: filterGroup
                onCheckedChanged: if (checked) window.selectedFilterMethod = 2
            }
        }
    }

    Component {
        id: rPeaksParams

        ColumnLayout {
            id: rPeaksRoot
            Layout.fillWidth: true
            spacing: 8

            Component.onCompleted: {
                if (window.selectedRPeaksMethod === 0) rbPanTompkins.checked = true
                else if (window.selectedRPeaksMethod === 1) rbHilbert.checked = true
                else if (window.selectedRPeaksMethod === 2) rbWavelet.checked = true
            }

            function isReady() {
                return ekgController.hasFilteredData && detectionMethodGroup.checkedButton !== null
            }

            function resetState() {
                detectionMethodGroup.checkedButton = null
                window.selectedRPeaksMethod = -1
            }

            function runDetection() {
                if (!detectionMethodGroup.checkedButton) {
                    showTemporaryStatus("⚠ Wybierz metode detekcji", Material.Orange)
                    return
                }

                analysisStatus.isProcessing = true
                chartLoading = true
                analysisProgress.value = 0

                if (rbPanTompkins.checked) {
                    window.selectedRPeaksMethod = 0
                    ekgController.runRPeaksDetection(0)
                } else if (rbHilbert.checked) {
                    window.selectedRPeaksMethod = 1
                    ekgController.runRPeaksDetection(1)
                } else if (rbWavelet.checked) {
                    window.selectedRPeaksMethod = 2
                    ekgController.runRPeaksDetection(2)
                }
            }

            Label {
                text: "Wybierz metode detekcji:"
                color: textSecondary
                font.pixelSize: 13
            }

            ButtonGroup {
                id: detectionMethodGroup
            }

            RadioButton {
                id: rbPanTompkins
                text: "Pan-Tompkins"
                ButtonGroup.group: detectionMethodGroup
                onCheckedChanged: if (checked) window.selectedRPeaksMethod = 0
            }

            RadioButton {
                id: rbHilbert
                text: "Transformata Hilberta"
                ButtonGroup.group: detectionMethodGroup
                onCheckedChanged: if (checked) window.selectedRPeaksMethod = 1
            }

            RadioButton {
                id: rbWavelet
                text: "Falkowa (Wavelet)"
                ButtonGroup.group: detectionMethodGroup
                onCheckedChanged: if (checked) window.selectedRPeaksMethod = 2
            }
        }
    }

    Component {
        id: hrvTimeParams

        ColumnLayout {
            id: hrvTimeRoot
            Layout.fillWidth: true
            spacing: 8

            Component.onCompleted: {
                if (window.selectedHRVTimeMethod === 0) rbClassicPeriodogram.checked = true
                else if (window.selectedHRVTimeMethod === 1) rbLombScargle.checked = true
                else if (window.selectedHRVTimeMethod === 2) rbWelch.checked = true
            }

            function isReady() {
                return ekgController.rPeaksCompleted && spectralMethodGroup.checkedButton !== null
            }

            function resetState() {
                spectralMethodGroup.checkedButton = null
                window.selectedHRVTimeMethod = -1
            }

            function runHRVTime() {
                if (!spectralMethodGroup.checkedButton) {
                    showTemporaryStatus("⚠ Wybierz metode estymacji", Material.Orange)
                    return
                }

                analysisStatus.isProcessing = true
                analysisProgress.value = 0

                if (rbClassicPeriodogram.checked) {
                    window.selectedHRVTimeMethod = 0
                    ekgController.runHRVTime(0)
                } else if (rbLombScargle.checked) {
                    window.selectedHRVTimeMethod = 1
                    ekgController.runHRVTime(1)
                } else if (rbWelch.checked) {
                    window.selectedHRVTimeMethod = 2
                    ekgController.runHRVTime(2)
                }
            }

            Label {
                text: "Metoda estymacji widma:"
                color: textSecondary
                font.pixelSize: 13
            }

            ButtonGroup {
                id: spectralMethodGroup
            }

            RadioButton {
                id: rbClassicPeriodogram
                text: "Klasyczny periodogram"
                ButtonGroup.group: spectralMethodGroup
                onCheckedChanged: if (checked) window.selectedHRVTimeMethod = 0
            }

            RadioButton {
                id: rbLombScargle
                text: "Lomb-Scargle"
                ButtonGroup.group: spectralMethodGroup
                onCheckedChanged: if (checked) window.selectedHRVTimeMethod = 1
            }

            RadioButton {
                id: rbWelch
                text: "Welch"
                ButtonGroup.group: spectralMethodGroup
                onCheckedChanged: if (checked) window.selectedHRVTimeMethod = 2
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: borderColor
                visible: ekgController.hrvTimeCompleted
                Layout.topMargin: 8
                Layout.bottomMargin: 8
            }

            GridLayout {
                visible: ekgController.hrvTimeCompleted
                columns: 2
                columnSpacing: 12
                rowSpacing: 6
                Layout.fillWidth: true

                Label {
                    text: "Metryki czasowe:"
                    font.bold: true
                    font.pixelSize: 14
                    Layout.columnSpan: 2
                }

                Label { text: "RR mean:" }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().rr_mean.toFixed(2) + " ms" : "-"
                    color: Material.color(Material.Teal)
                }

                Label { text: "SDNN:" }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().sdnn.toFixed(2) + " ms" : "-"
                    color: Material.color(Material.Teal)
                }

                Label { text: "RMSSD:" }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().rmssd.toFixed(2) + " ms" : "-"
                    color: Material.color(Material.Teal)
                }

                Label {
                    text: "Metryki czestotliwosciowe:"
                    font.bold: true
                    font.pixelSize: 14
                    Layout.columnSpan: 2
                    Layout.topMargin: 8
                }

                Label { text: "Total Power:" }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().tp.toFixed(2) + " ms²" : "-"
                    color: Material.color(Material.Teal)
                }

                Label { text: "VLF:" }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().vlf.toFixed(2) + " ms²" : "-"
                    color: Material.color(Material.Teal)
                }

                Label { text: "LF:" }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().lf.toFixed(2) + " ms²" : "-"
                    color: Material.color(Material.Teal)
                }

                Label { text: "HF:" }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().hf.toFixed(2) + " ms²" : "-"
                    color: Material.color(Material.Teal)
                }

                Label { text: "LF/HF:" }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().lf_hf.toFixed(3) : "-"
                    color: Material.color(Material.Teal)
                }
            }
        }
    }

    Component {
        id: hrvGeoParams

        ColumnLayout {
            id: hrvGeoRoot
            Layout.fillWidth: true
            spacing: 8

            function isReady() {
                return ekgController.rPeaksCompleted
            }

            function resetState() {
            }

            function runHRVGeo() {
                analysisStatus.isProcessing = true
                analysisProgress.value = 0
                ekgController.runHRVGeo()
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: borderColor
                visible: ekgController.hrvGeoCompleted
                Layout.topMargin: 8
                Layout.bottomMargin: 8
            }

            GridLayout {
                visible: ekgController.hrvGeoCompleted
                columns: 2
                columnSpacing: 12
                rowSpacing: 6
                Layout.fillWidth: true

                Label {
                    text: "Metryki trojkatne:"
                    font.bold: true
                    font.pixelSize: 14
                    Layout.columnSpan: 2
                }

                Label { text: "Triangular Index:" }
                Label {
                    text: ekgController.hrvGeoCompleted ? ekgController.getHRVGeoMetrics().triangular_index.toFixed(2) : "-"
                    color: Material.color(Material.Teal)
                }

                Label { text: "TINN:" }
                Label {
                    text: ekgController.hrvGeoCompleted ? ekgController.getHRVGeoMetrics().tinn.toFixed(2) + " ms" : "-"
                    color: Material.color(Material.Teal)
                }

                Label {
                    text: "Metryki Poincare:"
                    font.bold: true
                    font.pixelSize: 14
                    Layout.columnSpan: 2
                    Layout.topMargin: 8
                }

                Label { text: "SD1:" }
                Label {
                    text: ekgController.hrvGeoCompleted ? ekgController.getHRVGeoMetrics().sd1.toFixed(2) + " ms" : "-"
                    color: Material.color(Material.Teal)
                }

                Label { text: "SD2:" }
                Label {
                    text: ekgController.hrvGeoCompleted ? ekgController.getHRVGeoMetrics().sd2.toFixed(2) + " ms" : "-"
                    color: Material.color(Material.Teal)
                }
            }
        }
    }

    Component {
        id: wavesParams

        ColumnLayout {
            id: wavesRoot
            Layout.fillWidth: true
            spacing: 8

            function isReady() {
                return ekgController.hasFilteredData
            }

            function resetState() {
            }

            function runWaves() {
                analysisStatus.isProcessing = true
                chartLoading = true
                analysisProgress.value = 0
                ekgController.runWaves()
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: borderColor
                visible: ekgController.wavesCompleted
                Layout.topMargin: 8
                Layout.bottomMargin: 8
            }

            GridLayout {
                visible: ekgController.wavesCompleted
                columns: 2
                columnSpacing: 12
                rowSpacing: 6
                Layout.fillWidth: true

                Label {
                    text: "Wykryte znaczniki:"
                    font.bold: true
                    font.pixelSize: 14
                    Layout.columnSpan: 2
                }

                Label { text: "P onset:" }
                Label {
                    text: chartWaveMarkers.p_onsets ? chartWaveMarkers.p_onsets.length : "0"
                    color: Material.color(Material.Teal)
                }

                Label { text: "P end:" }
                Label {
                    text: chartWaveMarkers.p_ends ? chartWaveMarkers.p_ends.length : "0"
                    color: Material.color(Material.Teal)
                }

                Label { text: "QRS onset:" }
                Label {
                    text: chartWaveMarkers.qrs_onsets ? chartWaveMarkers.qrs_onsets.length : "0"
                    color: Material.color(Material.Teal)
                }

                Label { text: "QRS end:" }
                Label {
                    text: chartWaveMarkers.qrs_ends ? chartWaveMarkers.qrs_ends.length : "0"
                    color: Material.color(Material.Teal)
                }

                Label { text: "T end:" }
                Label {
                    text: chartWaveMarkers.t_ends ? chartWaveMarkers.t_ends.length : "0"
                    color: Material.color(Material.Teal)
                }
            }

            ColumnLayout {
                visible: ekgController.wavesCompleted
                Layout.fillWidth: true
                spacing: 4
                Layout.topMargin: 8

                Label {
                    text: "Legenda:"
                    font.bold: true
                    font.pixelSize: 14
                }

                RowLayout {
                    spacing: 8
                    Rectangle { width: 12; height: 12; radius: 6; color: "#f59e0b" }
                    Label { text: "P onset"; font.pixelSize: 12 }
                }

                RowLayout {
                    spacing: 8
                    Rectangle { width: 12; height: 12; radius: 6; color: "#eab308" }
                    Label { text: "P end"; font.pixelSize: 12 }
                }

                RowLayout {
                    spacing: 8
                    Rectangle { width: 12; height: 12; radius: 6; color: "#8b5cf6" }
                    Label { text: "QRS onset"; font.pixelSize: 12 }
                }

                RowLayout {
                    spacing: 8
                    Rectangle { width: 12; height: 12; radius: 6; color: "#a78bfa" }
                    Label { text: "QRS end"; font.pixelSize: 12 }
                }

                RowLayout {
                    spacing: 8
                    Rectangle { width: 12; height: 12; radius: 6; color: "#ec4899" }
                    Label { text: "T end"; font.pixelSize: 12 }
                }
            }
        }
    }

    Dialog {
        id: helpDialog
        title: "Jak korzystac z EKG Analyzer"
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
                text: "1. Wybierz plik EKG (Import sygnalu).\n" +
                      "2. Wybierz modul analizy (ECG BASELINE, R PEAKS, WAVES, HRV TIME lub HRV GEO).\n" +
                      "3. Ustaw parametry w prawym panelu.\n" +
                      "4. Kliknij 'Uruchom analize', aby przetworzyc sygnal.\n\n"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

    Dialog {
        id: loadConfirmDialog
        title: "Potwierdzenie"
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        implicitWidth: 400

        onVisibleChanged: if (visible) {
            x = (window.width  - implicitWidth)  / 2
            y = (window.height - implicitHeight) / 2
        }

        onAccepted: {
            ekgController.loadFileByName(pendingFileName)
            pendingFileName = ""
        }

        onRejected: {
            pendingFileName = ""
        }

        contentItem: ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 8

            Label {
                text: "Czy na pewno chcesz zaladowac plik " + pendingFileName + "?"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
}

