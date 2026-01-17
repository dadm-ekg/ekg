import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtCharts
import QtQml

ApplicationWindow {
    id: window
    width: 1280
    height: 720
    visible: true
    title: "EKG Analyzer"

    property bool isDarkTheme: true

    property color bgMain: isDarkTheme ? "#020712" : "#f3f4f6"
    property color panelColor: isDarkTheme ? "#050d18" : "#ffffff"
    property color borderColor: isDarkTheme ? "#1f2933" : "#d1d5db"
    property color textSecondary: isDarkTheme ? "#9ca3af" : "#4b5563"
    property color vizBg: isDarkTheme ? "#020812" : "#f9fafb"
    property color vizBorder: isDarkTheme ? "#0f172a" : "#d1d5db"

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
    property bool isProcessing: false
    property string pendingFileName: ""
    property string lastUsedFilter: ""
    property string lastUsedRPeaksMethod: ""
    property string lastUsedHRVTimeMethod: ""
    property bool hrvGeoRan: false
    property bool wavesRan: false
    property bool heartClassRan: false
    property int selectedFilterMethod: -1
    property int selectedRPeaksMethod: -1
    property int selectedHRVTimeMethod: -1
    property var chartWaveMarkers: {}
    property var heartClassAnnotations: []
    property real chartTotalDuration: 0
    property real chartWindowSize: 10.0
    property real chartScrollPosition: 0.0
    property real chartMinWindowSize: 1.0
    property real chartMaxWindowSize: 60.0
    property var loadedSettings: null

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
            chartTotalDuration = 0
            chartLoading = false
            applySeriesToChart()
            return
        }

        chartLoading = true
        
        var channel = clampChannelIndex(selectedChannelIndex)
        
        var duration = ekgController.signalDuration()
        if (duration > 0) {
            chartTotalDuration = duration
        }
        
        if (chartTotalDuration < 15) {
            chartRawSeries = ekgController.getRawSeries(channel, -1, -1, -1)
            chartFilteredSeries = ekgController.getFilteredSeries(channel, -1, -1, -1)
            chartRPeaksSeries = ekgController.getRPeakMarkers(channel, -1, -1)
            chartWaveMarkers = ekgController.getWaveMarkers(channel)
        } else {
            var effectiveWindowSize = Math.min(chartWindowSize, chartTotalDuration)
            var scrollableRange = Math.max(0, chartTotalDuration - effectiveWindowSize)
            var startTime = chartScrollPosition * scrollableRange
            var endTime = startTime + effectiveWindowSize
            
            if (chartTotalDuration > 0) {
                startTime = Math.max(0, startTime - 0.5)
                endTime = Math.min(chartTotalDuration, endTime + 0.5)
            }
            
            chartRawSeries = ekgController.getRawSeries(channel, maxPlottedPoints, startTime, endTime)
            chartFilteredSeries = ekgController.getFilteredSeries(channel, maxPlottedPoints, startTime, endTime)
            chartRPeaksSeries = ekgController.getRPeakMarkers(channel, startTime, endTime)
            chartWaveMarkers = ekgController.getWaveMarkers(channel)
        }
        
        applySeriesToChart()
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

        function scan(series) {
            for (var i = 0; i < series.length; ++i) {
                var p = series[i]
                if (!p) continue
                if (p.y < minY) minY = p.y
                if (p.y > maxY) maxY = p.y
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

        var effectiveWindowSize = Math.min(chartWindowSize, chartTotalDuration > 0 ? chartTotalDuration : 1)
        var scrollableRange = Math.max(0, chartTotalDuration - effectiveWindowSize)
        var startX = chartScrollPosition * scrollableRange
        var endX = startX + effectiveWindowSize

        chartAxisX.min = startX
        chartAxisX.max = endX
        chartAxisY.min = minY
        chartAxisY.max = maxY
    }

    function scrollChart(delta) {
        if (chartTotalDuration <= chartWindowSize) return
        var step = 0.05
        chartScrollPosition = Math.max(0, Math.min(1, chartScrollPosition + delta * step))
        updateChartView()
        if (chartTotalDuration >= 15) {
            scrollDebounce.restart()
        }
    }

    function zoomChart(factor) {
        var newWindowSize = chartWindowSize * factor
        newWindowSize = Math.max(chartMinWindowSize, Math.min(chartMaxWindowSize, newWindowSize))
        newWindowSize = Math.min(newWindowSize, chartTotalDuration)
        chartWindowSize = newWindowSize
        chartScrollPosition = Math.max(0, Math.min(1, chartScrollPosition))
        updateChartView()
        if (chartTotalDuration >= 15) {
            scrollDebounce.restart()
        }
    }

    function updateChartView() {
        var effectiveWindowSize = Math.min(chartWindowSize, chartTotalDuration)
        var scrollableRange = Math.max(0, chartTotalDuration - effectiveWindowSize)
        var startX = chartScrollPosition * scrollableRange
        var endX = startX + effectiveWindowSize
        chartAxisX.min = startX
        chartAxisX.max = endX
    }

    function resetChartView() {
        chartScrollPosition = 0
        chartWindowSize = Math.min(10.0, chartTotalDuration)
        updateChartView()
    }

    function updateAnalysisCharts() {
        if (window.currentModule === "HRV TIME" && ekgController.hrvTimeCompleted) {
            var powerSpectrum = ekgController.getHRVTimePowerSpectrum()
            powerSpectrumSeries.clear()
            var maxPower = 0
            for (var i = 0; i < powerSpectrum.length; i++) {
                var p = powerSpectrum[i]
                powerSpectrumSeries.append(p.x, p.y)
                if (p.y > maxPower) maxPower = p.y
            }
            hrvTimePowerAxis.max = maxPower * 1.1

            var tachogram = ekgController.getHRVTimeTachogram()
            tachogramSeries.clear()
            var maxRR = 0, minRR = Infinity
            var maxTime = 0
            for (var j = 0; j < tachogram.length; j++) {
                var t = tachogram[j]
                tachogramSeries.append(t.x, t.y)
                if (t.y > maxRR) maxRR = t.y
                if (t.y < minRR) minRR = t.y
                if (t.x > maxTime) maxTime = t.x
            }
            if (tachogram.length > 0) {
                hrvTimeTachogramTimeAxis.min = 0
                hrvTimeTachogramTimeAxis.max = maxTime
                hrvTimeTachogramRRAxis.min = Math.max(0, minRR * 0.9)
                hrvTimeTachogramRRAxis.max = maxRR * 1.1
            }
        } else if (window.currentModule === "HRV GEO" && ekgController.hrvGeoCompleted) {
            var histogram = ekgController.getHRVGeoHistogram()
            console.log("HRV GEO histogram length:", histogram ? histogram.length : 0)
            if (histogram && histogram.length > 0) {
                while (histogramBarSet.count > 0) {
                    histogramBarSet.remove(0)
                }
                var maxCount = 0
                var categories = []
                for (var k = 0; k < histogram.length; k++) {
                    var h = histogram[k]
                    if (h) {
                        var xVal = h.x !== undefined ? h.x : h["x"]
                        var yVal = h.y !== undefined ? h.y : h["y"]
                        if (xVal !== undefined && yVal !== undefined) {
                            histogramBarSet.append(yVal)
                            categories.push(xVal.toFixed(0))
                            if (yVal > maxCount) maxCount = yVal
                        }
                    }
                }
                hrvGeoHistogramAxisX.categories = categories
                console.log("HRV GEO histogram bars:", histogramBarSet.count, "maxCount:", maxCount)
                if (histogramBarSet.count > 0) {
                    hrvGeoHistogramAxisY.max = maxCount * 1.1
                    hrvGeoHistogramAxisY.min = 0
                }
            }

            var poincare = ekgController.getHRVGeoPoincare()
            console.log("HRV GEO poincare length:", poincare ? poincare.length : 0)
            if (poincare && poincare.length > 0) {
                poincareSeries.clear()
                var maxRRGeo = 0, minRRGeo = Infinity
                for (var l = 0; l < poincare.length; l++) {
                    var pc = poincare[l]
                    if (pc) {
                        var xVal = pc.x !== undefined ? pc.x : pc["x"]
                        var yVal = pc.y !== undefined ? pc.y : pc["y"]
                        if (xVal !== undefined && yVal !== undefined) {
                            poincareSeries.append(xVal, yVal)
                            if (xVal > maxRRGeo) maxRRGeo = xVal
                            if (yVal > maxRRGeo) maxRRGeo = yVal
                            if (xVal < minRRGeo) minRRGeo = xVal
                            if (yVal < minRRGeo) minRRGeo = yVal
                        }
                    }
                }
                console.log("HRV GEO poincare points:", poincareSeries.count, "min:", minRRGeo, "max:", maxRRGeo)
                if (poincareSeries.count > 0) {
                    var margin = (maxRRGeo - minRRGeo) * 0.1
                    hrvGeoPoincareAxisX.min = Math.max(0, minRRGeo - margin)
                    hrvGeoPoincareAxisX.max = maxRRGeo + margin
                    hrvGeoPoincareAxisY.min = Math.max(0, minRRGeo - margin)
                    hrvGeoPoincareAxisY.max = maxRRGeo + margin
                }
            }
        } else if (window.currentModule === "HEART CLASS" && ekgController.heartClassCompleted) {
            var barData = ekgController.getHeartClassBarChart()

            var values = [barData.N, barData.V, barData.A, barData.Other]
            heartClassBarSet.values = values

            heartClassBarCategoryAxis.categories = ["N", "V", "A", "Inne"]

            var maxBar = Math.max(barData.N, Math.max(barData.V, Math.max(barData.A, barData.Other)))
            if (maxBar > 0) {
                heartClassBarAxisY.max = maxBar * 1.1
                heartClassBarAxisY.min = 0
            }
        }
    }

    property string currentModule: "ECG BASELINE"
    onCurrentModuleChanged: {
        window.isProcessing = false
        analysisProgress.value = 0
        updateMarkerVisibility()
        Qt.callLater(updateAnalysisCharts)
    }

    function applyLoadedSettings() {
        if (!loadedSettings) return
        
        var settings = loadedSettings
        
        if (paramsLoader.item) {
            if (window.currentModule === "ECG BASELINE") {
                if ("selectedFilterMethod" in settings) {
                    var filterMethod = settings["selectedFilterMethod"]
                    if (filterMethod === 0 && paramsLoader.item.rbMovingAverage) {
                        paramsLoader.item.rbMovingAverage.checked = true
                    } else if (filterMethod === 1 && paramsLoader.item.rbButterworth) {
                        paramsLoader.item.rbButterworth.checked = true
                    } else if (filterMethod === 2 && paramsLoader.item.rbSavitzkyGolay) {
                        paramsLoader.item.rbSavitzkyGolay.checked = true
                    }
                }
                if ("windowSize" in settings) {
                    paramsLoader.item.windowSize = settings["windowSize"]
                }
                if ("polynomialOrder" in settings) {
                    paramsLoader.item.polynomialOrder = settings["polynomialOrder"]
                }
            } else if (window.currentModule === "R PEAKS") {
                if ("selectedRPeaksMethod" in settings) {
                    var rpeaksMethod = settings["selectedRPeaksMethod"]
                    if (rpeaksMethod === 0 && paramsLoader.item.rbPanTompkins) {
                        paramsLoader.item.rbPanTompkins.checked = true
                    } else if (rpeaksMethod === 1 && paramsLoader.item.rbHilbert) {
                        paramsLoader.item.rbHilbert.checked = true
                    } else if (rpeaksMethod === 2 && paramsLoader.item.rbWavelet) {
                        paramsLoader.item.rbWavelet.checked = true
                    }
                }
            } else if (window.currentModule === "HRV TIME") {
                if ("selectedHRVTimeMethod" in settings) {
                    var hrvMethod = settings["selectedHRVTimeMethod"]
                    if (hrvMethod === 0 && paramsLoader.item.rbClassicPeriodogram) {
                        paramsLoader.item.rbClassicPeriodogram.checked = true
                    } else if (hrvMethod === 1 && paramsLoader.item.rbLombScargle) {
                        paramsLoader.item.rbLombScargle.checked = true
                    } else if (hrvMethod === 2 && paramsLoader.item.rbWelch) {
                        paramsLoader.item.rbWelch.checked = true
                    }
                }
            }
        }
        
        loadedSettings = null
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
            window.isProcessing = false
            lastUsedFilter = ""
            lastUsedRPeaksMethod = ""
            lastUsedHRVTimeMethod = ""
            selectedFilterMethod = -1
            selectedRPeaksMethod = -1
            selectedHRVTimeMethod = -1
            hrvGeoRan = false
            wavesRan = false
            heartClassRan = false
            chartWaveMarkers = {}
            heartClassAnnotations = []
            chartRawSeries = []
            chartFilteredSeries = []
            chartRPeaksSeries = []
            chartScrollPosition = 0
            chartWindowSize = 10.0
            moduleCombo.currentIndex = 0
            rebuildChannelOptions()
            refreshVisualization()
        }

        function onFileLoadError(errorMessage) {
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }

        function onFilteringSuccess(filterName) {
            lastUsedFilter = filterName
            window.isProcessing = false
            analysisProgress.value = 100
            refreshVisualization()
        }

        function onFilteringError(errorMessage) {
            window.isProcessing = false
            chartLoading = false
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }

        function onRPeaksDetectionSuccess(methodName) {
            lastUsedRPeaksMethod = methodName
            window.isProcessing = false
            analysisProgress.value = 100
            refreshVisualization()
        }

        function onBaselineCompletedChanged() {
            if (ekgController.baselineCompleted) {
                window.isProcessing = false
                analysisProgress.value = 100
                refreshVisualization()
            }
        }

        function onRPeaksCompletedChanged() {
            if (ekgController.rPeaksCompleted) {
                window.isProcessing = false
                analysisProgress.value = 100
                refreshVisualization()
            }
        }

        function onRPeaksDetectionError(errorMessage) {
            window.isProcessing = false
            chartLoading = false
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }

        function onHrvTimeSuccess(methodName) {
            lastUsedHRVTimeMethod = methodName
            window.isProcessing = false
            analysisProgress.value = 100
            Qt.callLater(updateAnalysisCharts)
        }

        function onHrvTimeError(errorMessage) {
            window.isProcessing = false
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }

        function onHrvTimeCompletedChanged() {
            if (ekgController.hrvTimeCompleted) {
                window.isProcessing = false
                analysisProgress.value = 100
                if (window.currentModule === "HRV TIME") {
                    Qt.callLater(updateAnalysisCharts)
                }
            }
        }

        function onHrvGeoSuccess() {
            hrvGeoRan = true
            window.isProcessing = false
            analysisProgress.value = 100
            Qt.callLater(updateAnalysisCharts)
        }

        function onHrvGeoError(errorMessage) {
            window.isProcessing = false
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }

        function onHrvGeoCompletedChanged() {
            if (ekgController.hrvGeoCompleted) {
                window.isProcessing = false
                analysisProgress.value = 100
                if (window.currentModule === "HRV GEO") {
                    Qt.callLater(updateAnalysisCharts)
                }
            }
        }

        function onWavesSuccess() {
            wavesRan = true
            window.isProcessing = false
            analysisProgress.value = 100
            refreshVisualization()
        }

        function onWavesError(errorMessage) {
            window.isProcessing = false
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }

        function onWavesCompletedChanged() {
            if (ekgController.wavesCompleted) {
                window.isProcessing = false
                analysisProgress.value = 100
                refreshVisualization()
            }
        }

        function onHeartClassSuccess() {
            heartClassRan = true
            heartClassAnnotations = ekgController.getHeartClassAnnotations()
            window.isProcessing = false
            analysisProgress.value = 100
            Qt.callLater(updateAnalysisCharts)
        }

        function onHeartClassError(errorMessage) {
            window.isProcessing = false
            analysisProgress.value = 0
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }

        function onHeartClassCompletedChanged() {
            if (ekgController.heartClassCompleted) {
                heartClassAnnotations = ekgController.getHeartClassAnnotations()
                window.isProcessing = false
                analysisProgress.value = 100
                if (window.currentModule === "HEART CLASS") {
                    Qt.callLater(updateAnalysisCharts)
                }
            }
        }

        function onSettingsSaveSuccess(filepath) {
            showTemporaryStatus("✓ Ustawienia zapisane", Material.Green)
        }

        function onSettingsSaveError(errorMessage) {
            showTemporaryStatus("✗ " + errorMessage, Material.Red)
        }

        function onSettingsLoadSuccess(settings) {
            console.log("Settings loaded:", JSON.stringify(settings))
            
            window.loadedSettings = settings
            
            if ("isDarkTheme" in settings) {
                window.isDarkTheme = settings["isDarkTheme"]
            }
            if ("chartWindowSize" in settings) {
                window.chartWindowSize = settings["chartWindowSize"]
            }
            if ("maxPlottedPoints" in settings) {
                window.maxPlottedPoints = settings["maxPlottedPoints"]
            }
            if ("selectedChannel" in settings) {
                window.selectedChannelIndex = settings["selectedChannel"]
            }
            if ("selectedFilterMethod" in settings) {
                window.selectedFilterMethod = settings["selectedFilterMethod"]
            }
            if ("selectedRPeaksMethod" in settings) {
                window.selectedRPeaksMethod = settings["selectedRPeaksMethod"]
            }
            if ("selectedHRVTimeMethod" in settings) {
                window.selectedHRVTimeMethod = settings["selectedHRVTimeMethod"]
            }
            
            if ("currentModule" in settings) {
                var modules = ["ECG BASELINE", "R PEAKS", "WAVES", "HRV TIME", "HRV GEO", "HEART CLASS"]
                var idx = modules.indexOf(settings["currentModule"])
                if (idx >= 0) {
                    moduleCombo.currentIndex = idx
                }
            }
            
            Qt.callLater(function() {
                applyLoadedSettings()
            })
            
            showTemporaryStatus("✓ Ustawienia wczytane", Material.Green)
            refreshVisualization()
        }

        function onSettingsLoadError(errorMessage) {
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

    Timer {
        id: vizDebounce
        interval: 150
        repeat: false
        onTriggered: refreshVisualization()
    }
    
    Timer {
        id: scrollDebounce
        interval: 300
        repeat: false
        onTriggered: refreshVisualization()
    }

    header: ToolBar {
        leftPadding: 8
        rightPadding: 8
        topPadding: 0
        bottomPadding: 0
        RowLayout {
            anchors.fill: parent
            spacing: 16

            Label {
                text: "EKG Analyzer"
                font.pixelSize: 22
                font.bold: true
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 16
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: "Import sygnału"
                icon.name: "document-open"
                Material.foreground: window.buttonTextColor
                onClicked: ekgController.openFileDialog()
            }

            Button {
                id: settingsButton
                text: "Ustawienia"
                Material.foreground: window.buttonTextColor
                Layout.alignment: Qt.AlignVCenter

                Menu {
                    id: settingsMenu
                    x: settingsButton.x
                    y: settingsButton.y + settingsButton.height

                    MenuItem {
                        text: "Wyeksportuj ustawienia"
                        onTriggered: {
                            var windowSize = 5
                            var polynomialOrder = 2
                            if (window.currentModule === "ECG BASELINE" && paramsLoader.item) {
                                if (paramsLoader.item.windowSize !== undefined) {
                                    windowSize = paramsLoader.item.windowSize
                                }
                                if (paramsLoader.item.polynomialOrder !== undefined) {
                                    polynomialOrder = paramsLoader.item.polynomialOrder
                                }
                            }
                            ekgController.openSaveSettingsDialog(
                                window.selectedFilterMethod,
                                window.selectedRPeaksMethod,
                                window.selectedHRVTimeMethod,
                                window.selectedChannelIndex,
                                window.currentModule,
                                window.isDarkTheme,
                                window.chartWindowSize,
                                window.maxPlottedPoints,
                                windowSize,
                                polynomialOrder
                            )
                        }
                    }
                    MenuItem {
                        text: "Załaduj ustawienia"
                        onTriggered: {
                            ekgController.openLoadSettingsDialog()
                        }
                    }
                }

                onClicked: {
                    settingsMenu.open()
                }
            }

            Button {
                id: saveResultsButton
                text: "Zapisz wyniki"
                icon.name: "document-save"
                Material.foreground: window.buttonTextColor
                enabled: {
                    if (window.currentModule === "ECG BASELINE") {
                        return ekgController.baselineCompleted
                    } else if (window.currentModule === "R PEAKS") {
                        return ekgController.rPeaksCompleted
                    } else if (window.currentModule === "HRV TIME") {
                        return ekgController.hrvTimeCompleted
                    } else if (window.currentModule === "HRV GEO") {
                        return ekgController.hrvGeoCompleted
                    } else if (window.currentModule === "WAVES") {
                        return ekgController.wavesCompleted
                    } else if (window.currentModule === "HEART CLASS") {
                        return ekgController.heartClassCompleted
                    }
                    return false
                }

                Menu {
                    id: saveResultsMenu
                    x: saveResultsButton.x
                    y: saveResultsButton.y + saveResultsButton.height

                    MenuItem {
                        text: "Zapisz jako CSV"
                        onTriggered: {
                            var baseName = ekgController.loadedFilename
                            if (baseName === "") baseName = "wyniki"
                            else {
                                var lastDot = baseName.lastIndexOf(".")
                                if (lastDot > 0) baseName = baseName.substring(0, lastDot)
                            }
                            var moduleName = window.currentModule.replace(/\s+/g, "_").toLowerCase()
                            var defaultFileName = baseName + "_" + moduleName + ".csv"
                            exportFileDialog.nameFilters = ["CSV files (*.csv)", "All files (*)"]
                            exportFileDialog.currentFile = defaultFileName
                            exportFileDialog.selectedFormat = "csv"
                            exportFileDialog.open()
                        }
                    }
                    MenuItem {
                        text: "Zapisz jako HTML"
                        onTriggered: {
                            var baseName = ekgController.loadedFilename
                            if (baseName === "") baseName = "wyniki"
                            else {
                                var lastDot = baseName.lastIndexOf(".")
                                if (lastDot > 0) baseName = baseName.substring(0, lastDot)
                            }
                            var moduleName = window.currentModule.replace(/\s+/g, "_").toLowerCase()
                            var defaultFileName = baseName + "_" + moduleName + ".html"
                            exportFileDialog.nameFilters = ["HTML files (*.html)", "All files (*)"]
                            exportFileDialog.currentFile = defaultFileName
                            exportFileDialog.selectedFormat = "html"
                            exportFileDialog.open()
                        }
                    }
                    MenuItem {
                        text: "Zapisz jako JSON"
                        onTriggered: {
                            var baseName = ekgController.loadedFilename
                            if (baseName === "") baseName = "wyniki"
                            else {
                                var lastDot = baseName.lastIndexOf(".")
                                if (lastDot > 0) baseName = baseName.substring(0, lastDot)
                            }
                            var moduleName = window.currentModule.replace(/\s+/g, "_").toLowerCase()
                            var defaultFileName = baseName + "_" + moduleName + ".json"
                            exportFileDialog.nameFilters = ["JSON files (*.json)", "All files (*)"]
                            exportFileDialog.currentFile = defaultFileName
                            exportFileDialog.selectedFormat = "json"
                            exportFileDialog.open()
                        }
                    }
                }

                onClicked: {
                    saveResultsMenu.open()
                }
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
                        NumberAnimation {
                            duration: 160; easing.type: Easing.InOutQuad
                        }
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

                    section.property: "modelData"
                    section.criteria: ViewSection.FirstCharacter
                    section.delegate: Item {
                        width: ListView.view.width
                        height: 28
                        
                        Rectangle {
                            anchors.fill: parent
                            color: isDarkTheme ? "#1a2332" : "#e5e7eb"
                            
                            Label {
                                anchors.left: parent.left
                                anchors.leftMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                text: {
                                    var sec = section
                                    if (sec === "L") return "📁 LUDB"
                                    if (sec === "M") return "📁 MIT-BIH"
                                    return sec
                                }
                                font.bold: true
                                font.pixelSize: 12
                                color: isDarkTheme ? "#60a5fa" : "#2563eb"
                            }
                        }
                    }

                    delegate: ItemDelegate {
                        width: ListView.view.width
                        
                        contentItem: Row {
                            spacing: 8
                            
                            Rectangle {
                                width: 48
                                height: 18
                                radius: 3
                                anchors.verticalCenter: parent.verticalCenter
                                color: modelData.startsWith("LUDB/") ? (isDarkTheme ? "#1e3a5f" : "#dbeafe") : (isDarkTheme ? "#3f1e3f" : "#fce7f3")
                                
                                Label {
                                    anchors.centerIn: parent
                                    text: modelData.startsWith("LUDB/") ? "LUDB" : "MITBIH"
                                    font.pixelSize: 10
                                    font.bold: true
                                    color: modelData.startsWith("LUDB/") ? (isDarkTheme ? "#60a5fa" : "#2563eb") : (isDarkTheme ? "#f472b6" : "#db2777")
                                }
                            }
                            
                            Label {
                                text: {
                                    if (modelData.startsWith("LUDB/")) return modelData.substring(5)
                                    if (modelData.startsWith("MITBIH/")) return modelData.substring(7)
                                    return modelData
                                }
                                anchors.verticalCenter: parent.verticalCenter
                                color: isDarkTheme ? "#f9fafb" : "#111827"
                            }
                        }
                        
                        highlighted: {
                            var loadedName = ekgController.loadedFilename
                            if (modelData.startsWith("LUDB/")) return loadedName === modelData.substring(5)
                            if (modelData.startsWith("MITBIH/")) return loadedName === modelData.substring(7)
                            return loadedName === modelData
                        }

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
                    enabled: !window.isProcessing && !chartLoading
                    model: [
                        "ECG BASELINE",
                        "R PEAKS",
                        "WAVES",
                        "HRV TIME",
                        "HRV GEO",
                        "HEART CLASS"
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
                                enabled: model.length > 0 && !chartLoading && window.currentModule !== "HRV TIME" && window.currentModule !== "HRV GEO" && window.currentModule !== "HEART CLASS"
                                visible: window.currentModule !== "HRV TIME" && window.currentModule !== "HRV GEO" && window.currentModule !== "HEART CLASS"
                                currentIndex: selectedChannelIndex
                                onActivated: {
                                    selectedChannelIndex = currentIndex
                                    chartLoading = true
                                    scheduleVisualizationRefresh()
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Button {
                                text: "Oddal"
                                enabled: ekgController.hasData && window.currentModule !== "HRV TIME" && window.currentModule !== "HRV GEO" && window.currentModule !== "HEART CLASS"
                                visible: window.currentModule !== "HRV TIME" && window.currentModule !== "HRV GEO" && window.currentModule !== "HEART CLASS"
                                onClicked: zoomChart(1.5)
                            }

                            Button {
                                text: "Przybliż"
                                enabled: ekgController.hasData && window.currentModule !== "HRV TIME" && window.currentModule !== "HRV GEO" && window.currentModule !== "HEART CLASS"
                                visible: window.currentModule !== "HRV TIME" && window.currentModule !== "HRV GEO" && window.currentModule !== "HEART CLASS"
                                onClicked: zoomChart(0.67)
                            }
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 40
                            spacing: 10
                            visible: ekgController.hasData && chartTotalDuration > chartWindowSize && window.currentModule !== "HRV TIME" && window.currentModule !== "HRV GEO" && window.currentModule !== "HEART CLASS"
                            
                            Label {
                                text: {
                                    var effectiveWindowSize = Math.min(chartWindowSize, chartTotalDuration)
                                    var scrollableRange = Math.max(0, chartTotalDuration - effectiveWindowSize)
                                    var startTime = chartScrollPosition * scrollableRange
                                    var minutes = Math.floor(startTime / 60)
                                    var seconds = Math.floor(startTime % 60)
                                    return minutes.toString().padStart(2, '0') + ":" + seconds.toString().padStart(2, '0')
                                }
                                font.family: "monospace"
                                color: textSecondary
                            }
                            
                            Slider {
                                id: navigationSlider
                                Layout.fillWidth: true
                                from: 0
                                to: 1
                                value: chartScrollPosition
                                enabled: chartTotalDuration > chartWindowSize
                                
                                onMoved: {
                                    chartScrollPosition = value
                                    updateChartView()
                                }
                                
                                onPressedChanged: {
                                    if (!pressed && chartTotalDuration >= 15) {
                                        refreshVisualization()
                                    }
                                }
                            }
                            
                            Label {
                                text: {
                                    var minutes = Math.floor(chartTotalDuration / 60)
                                    var seconds = Math.floor(chartTotalDuration % 60)
                                    return minutes.toString().padStart(2, '0') + ":" + seconds.toString().padStart(2, '0')
                                }
                                font.family: "monospace"
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
                                legend.visible: window.currentModule !== "WAVES"
                                legend.alignment: Qt.AlignTop
                                legend.labelColor: textSecondary
                                enabled: ekgController.hasData
                                visible: window.currentModule !== "HRV TIME" && window.currentModule !== "HRV GEO" && window.currentModule !== "HEART CLASS"

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

                            Rectangle {
                                id: chartTooltip
                                visible: false
                                width: tooltipContent.width + 16
                                height: tooltipContent.height + 12
                                radius: 6
                                color: isDarkTheme ? "#1f2937" : "#ffffff"
                                border.color: isDarkTheme ? "#374151" : "#d1d5db"
                                border.width: 1
                                z: 100

                                property string labelText: ""
                                property color labelColor: "#ffffff"

                                function showTooltip(label, xVal, yVal, posX, posY, color) {
                                    labelText = label
                                    labelColor = color
                                    tooltipTime.text = "Czas: " + xVal.toFixed(3) + " s"
                                    tooltipValue.text = "Wartość: " + yVal.toFixed(3) + " mV"
                                    x = Math.min(posX + 10, parent.width - width - 10)
                                    y = Math.max(posY - height - 10, 10)
                                    visible = true
                                }

                                function hide() {
                                    visible = false
                                }

                                ColumnLayout {
                                    id: tooltipContent
                                    anchors.centerIn: parent
                                    spacing: 2

                                    Label {
                                        id: tooltipLabel
                                        text: chartTooltip.labelText
                                        font.bold: true
                                        font.pixelSize: 12
                                        color: chartTooltip.labelColor
                                    }

                                    Label {
                                        id: tooltipTime
                                        font.pixelSize: 11
                                        color: textSecondary
                                    }

                                    Label {
                                        id: tooltipValue
                                        font.pixelSize: 11
                                        color: textSecondary
                                    }
                                }
                            }

                            MouseArea {
                                id: chartMouseArea
                                anchors.fill: parent
                                acceptedButtons: Qt.LeftButton
                                hoverEnabled: true
                                cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor

                                property real dragStartX: 0
                                property real dragStartScrollPos: 0

                                function findNearestPoint(mouseX, mouseY) {
                                    var chartArea = signalChart.plotArea
                                    if (!chartArea || chartArea.width <= 0) return null

                                    var relX = mouseX - chartArea.x
                                    var relY = mouseY - chartArea.y

                                    if (relX < 0 || relX > chartArea.width || relY < 0 || relY > chartArea.height) {
                                        return null
                                    }

                                    var xMin = chartAxisX.min
                                    var xMax = chartAxisX.max
                                    var yMin = chartAxisY.min
                                    var yMax = chartAxisY.max

                                    var timeAtMouse = xMin + (relX / chartArea.width) * (xMax - xMin)
                                    var valueAtMouse = yMax - (relY / chartArea.height) * (yMax - yMin)

                                    var bestPoint = null
                                    var bestDist = 20

                                    function checkSeries(series, label, color) {
                                        if (!series.visible) return
                                        for (var i = 0; i < series.count; i++) {
                                            var pt = series.at(i)
                                            var screenX = chartArea.x + (pt.x - xMin) / (xMax - xMin) * chartArea.width
                                            var screenY = chartArea.y + (yMax - pt.y) / (yMax - yMin) * chartArea.height
                                            var dist = Math.sqrt(Math.pow(mouseX - screenX, 2) + Math.pow(mouseY - screenY, 2))
                                            if (dist < bestDist) {
                                                bestDist = dist
                                                bestPoint = {
                                                    x: pt.x,
                                                    y: pt.y,
                                                    screenX: screenX,
                                                    screenY: screenY,
                                                    label: label,
                                                    color: color
                                                }
                                            }
                                        }
                                    }

                                    if (window.currentModule === "R PEAKS") {
                                        checkSeries(peaksSeries, "Pik R", "#ef4444")
                                    } else if (window.currentModule === "WAVES") {
                                        checkSeries(pOnsetSeries, "P onset", "#f59e0b")
                                        checkSeries(pEndSeries, "P end", "#eab308")
                                        checkSeries(qrsOnsetSeries, "QRS onset", "#8b5cf6")
                                        checkSeries(qrsEndSeries, "QRS end", "#a78bfa")
                                        checkSeries(tEndSeries, "T end", "#ec4899")
                                    }

                                    return bestPoint
                                }

                                onPressed: function (mouse) {
                                    dragStartX = mouse.x
                                    dragStartScrollPos = chartScrollPosition
                                    chartTooltip.hide()
                                }
                                
                                onReleased: function (mouse) {
                                    if (Math.abs(chartScrollPosition - dragStartScrollPos) > 0.001 && chartTotalDuration >= 15) {
                                        refreshVisualization()
                                    }
                                }

                                onPositionChanged: function (mouse) {
                                    if (pressed && chartTotalDuration > chartWindowSize) {
                                        var deltaX = mouse.x - dragStartX
                                        var chartWidth = width
                                        var scrollableRange = chartTotalDuration - chartWindowSize
                                        var deltaNormalized = -deltaX / chartWidth * (chartWindowSize / scrollableRange)
                                        chartScrollPosition = Math.max(0, Math.min(1, dragStartScrollPos + deltaNormalized))
                                        updateChartView()
                                    } else if (!pressed) {
                                        var point = findNearestPoint(mouse.x, mouse.y)
                                        if (point) {
                                            chartTooltip.showTooltip(point.label, point.x, point.y, point.screenX, point.screenY, point.color)
                                        } else {
                                            chartTooltip.hide()
                                        }
                                    }
                                }

                                onExited: {
                                    chartTooltip.hide()
                                }

                                onWheel: function (wheel) {
                                    if (wheel.modifiers & Qt.ControlModifier) {
                                        if (wheel.angleDelta.y > 0) {
                                            zoomChart(0.8)
                                        } else {
                                            zoomChart(1.25)
                                        }
                                    } else {
                                        if (wheel.angleDelta.y > 0) {
                                            scrollChart(-1)
                                        } else {
                                            scrollChart(1)
                                        }
                                    }
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

                            ColumnLayout {
                                anchors.fill: parent
                                visible: window.currentModule === "HRV TIME" && ekgController.hrvTimeCompleted
                                spacing: 8

                                Label {
                                    text: "Widmo mocy"
                                    font.bold: true
                                    font.pixelSize: 14
                                    Layout.fillWidth: true
                                }

                                ChartView {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: parent.height * 0.5 - 20
                                    antialiasing: true
                                    theme: isDarkTheme ? ChartView.ChartThemeDark : ChartView.ChartThemeLight
                                    backgroundColor: vizBg
                                    legend.visible: false

                                    ValueAxis {
                                        id: hrvTimeFreqAxis
                                        titleText: "Częstotliwość [Hz]"
                                        labelsColor: textSecondary
                                        min: 0
                                        max: 2.0
                                    }

                                    ValueAxis {
                                        id: hrvTimePowerAxis
                                        titleText: "Moc widma [ms²/Hz]"
                                        labelsColor: textSecondary
                                    }

                                    LineSeries {
                                        id: powerSpectrumSeries
                                        axisX: hrvTimeFreqAxis
                                        axisY: hrvTimePowerAxis
                                        color: "#10b981"
                                        width: 2
                                    }
                                }

                                Label {
                                    text: "Tachogram RR"
                                    font.bold: true
                                    font.pixelSize: 14
                                    Layout.fillWidth: true
                                }

                                ChartView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    antialiasing: true
                                    theme: isDarkTheme ? ChartView.ChartThemeDark : ChartView.ChartThemeLight
                                    backgroundColor: vizBg
                                    legend.visible: false

                                    ValueAxis {
                                        id: hrvTimeTachogramTimeAxis
                                        titleText: "Czas [s]"
                                        labelsColor: textSecondary
                                    }

                                    ValueAxis {
                                        id: hrvTimeTachogramRRAxis
                                        titleText: "RR [ms]"
                                        labelsColor: textSecondary
                                    }

                                    LineSeries {
                                        id: tachogramSeries
                                        axisX: hrvTimeTachogramTimeAxis
                                        axisY: hrvTimeTachogramRRAxis
                                        color: "#60a5fa"
                                        width: 2
                                    }
                                }
                            }

                            ColumnLayout {
                                anchors.fill: parent
                                visible: window.currentModule === "HRV GEO" && ekgController.hrvGeoCompleted
                                spacing: 8

                                Label {
                                    text: "Histogram interwałów RR"
                                    font.bold: true
                                    font.pixelSize: 14
                                    Layout.fillWidth: true
                                }

                                ChartView {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: parent.height * 0.5 - 20
                                    antialiasing: true
                                    theme: isDarkTheme ? ChartView.ChartThemeDark : ChartView.ChartThemeLight
                                    backgroundColor: vizBg
                                    legend.visible: false

                                    BarCategoryAxis {
                                        id: hrvGeoHistogramAxisX
                                        titleText: "RR [ms]"
                                        labelsColor: textSecondary
                                        labelsAngle: -45
                                    }

                                    ValueAxis {
                                        id: hrvGeoHistogramAxisY
                                        titleText: "Liczba"
                                        labelsColor: textSecondary
                                    }

                                    BarSeries {
                                        id: histogramBarSeries
                                        axisX: hrvGeoHistogramAxisX
                                        axisY: hrvGeoHistogramAxisY
                                        barWidth: 0.9
                                        
                                        BarSet {
                                            id: histogramBarSet
                                            label: "RR"
                                            color: "#10b981"
                                        }
                                    }
                                }

                                Label {
                                    text: "Wykres Poincaré"
                                    font.bold: true
                                    font.pixelSize: 14
                                    Layout.fillWidth: true
                                }

                                ChartView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    antialiasing: true
                                    theme: isDarkTheme ? ChartView.ChartThemeDark : ChartView.ChartThemeLight
                                    backgroundColor: vizBg
                                    legend.visible: false

                                    ValueAxis {
                                        id: hrvGeoPoincareAxisX
                                        titleText: "RR(n) [ms]"
                                        labelsColor: textSecondary
                                    }

                                    ValueAxis {
                                        id: hrvGeoPoincareAxisY
                                        titleText: "RR(n+1) [ms]"
                                        labelsColor: textSecondary
                                    }

                                    ScatterSeries {
                                        id: poincareSeries
                                        axisX: hrvGeoPoincareAxisX
                                        axisY: hrvGeoPoincareAxisY
                                        color: "#8b5cf6"
                                        markerSize: 4
                                    }
                                }
                            }

                            ColumnLayout {
                                anchors.fill: parent
                                visible: window.currentModule === "HEART CLASS" && ekgController.heartClassCompleted
                                spacing: 8

                                Label {
                                    text: "Liczność klas QRS"
                                    font.bold: true
                                    font.pixelSize: 14
                                    Layout.fillWidth: true
                                }

                                ChartView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    antialiasing: true
                                    theme: isDarkTheme ? ChartView.ChartThemeDark : ChartView.ChartThemeLight
                                    backgroundColor: vizBg
                                    legend.visible: true
                                    legend.alignment: Qt.AlignTop
                                    legend.labelColor: textSecondary

                                    BarCategoryAxis {
                                        id: heartClassBarCategoryAxis
                                    }

                                    ValueAxis {
                                        id: heartClassBarAxisY
                                        titleText: "Liczba"
                                        labelsColor: textSecondary
                                    }

                                    BarSeries {
                                        id: heartClassBarSeries
                                        axisX: heartClassBarCategoryAxis
                                        axisY: heartClassBarAxisY

                                        BarSet {
                                            id: heartClassBarSet
                                            label: "Klasy"
                                        }
                                    }
                                }
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

            ScrollView {
                id: rightPanelScroll
                anchors.fill: parent
                anchors.margins: 12
                ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                clip: true

                ColumnLayout {
                    width: Math.max(rightPanelScroll.width - 24, implicitWidth)
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: 10

                    Label {
                    text: window.currentModule
                    font.bold: true
                    font.pixelSize: 18
                }

                Label {
                    visible: !ekgController.hasData && window.currentModule === "ECG BASELINE"
                    text: "⚠️ Najpierw zaimportuj sygnał EKG"
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
                    text: "⚠️ Najpierw uruchom detekcję pików R"
                    color: Material.color(Material.Orange)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Label {
                    visible: !ekgController.rPeaksCompleted && window.currentModule === "HRV GEO"
                    text: "⚠️ Najpierw uruchom detekcję pików R"
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

                Label {
                    // visible: !ekgController.hasFilteredData && window.currentModule === "HEART CLASS"
                    visible: !ekgController.rPeaksCompleted && window.currentModule === "HEART CLASS"
                    text: "⚠️ Najpierw uruchom detekcję pików R"
                    color: Material.color(Material.Orange)
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Label {
                        id: analysisStatus
                        property string statusText: {
                            var module = window.currentModule
                            var processing = window.isProcessing
                            var hasData = ekgController.hasData
                            var hasFiltered = ekgController.hasFilteredData
                            var baselineOK = ekgController.baselineCompleted
                            var rPeaksOK = ekgController.rPeaksCompleted
                            var hrvTimeOK = ekgController.hrvTimeCompleted

                            if (module === "ECG BASELINE") {
                                if (!hasData) {
                                    return "Oczekiwanie na plik"
                                } else if (processing) {
                                    return "Przetwarzanie..."
                                } else if (!baselineOK) {
                                    return "Gotowy"
                                } else {
                                    if (lastUsedFilter !== "") {
                                        return "Przefiltrowano z użyciem " + lastUsedFilter
                                    } else {
                                        return "Skończono"
                                    }
                                }
                            } else if (module === "R PEAKS") {
                                if (!hasData) {
                                    return "Oczekiwanie na plik"
                                } else if (!hasFiltered) {
                                    return "Oczekiwanie na filtrowanie"
                                } else if (processing) {
                                    return "Przetwarzanie..."
                                } else if (!rPeaksOK) {
                                    return "Gotowy"
                                } else {
                                    if (lastUsedRPeaksMethod !== "") {
                                        return "Wykryto piki R metodą " + lastUsedRPeaksMethod
                                    } else {
                                        return "Skończono"
                                    }
                                }
                            } else if (module === "HRV TIME") {
                                if (!hasData) {
                                    return "Oczekiwanie na plik"
                                } else if (!rPeaksOK) {
                                    return "Oczekiwanie na detekcje R"
                                } else if (processing) {
                                    return "Przetwarzanie..."
                                } else if (!hrvTimeOK) {
                                    return "Gotowy"
                                } else {
                                    if (lastUsedHRVTimeMethod !== "") {
                                        return "Obliczono metodą " + lastUsedHRVTimeMethod
                                    } else {
                                        return "Skończono"
                                    }
                                }
                            } else if (module === "HRV GEO") {
                                var hrvGeoOK = ekgController.hrvGeoCompleted
                                if (!hasData) {
                                    return "Oczekiwanie na plik"
                                } else if (!rPeaksOK) {
                                    return "Oczekiwanie na detekcje R"
                                } else if (processing) {
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
                                } else if (processing) {
                                    return "Przetwarzanie..."
                                } else if (!wavesOK) {
                                    return "Gotowy"
                                } else {
                                    return "Wykryto fale EKG"
                                }
                            } else if (module === "HEART CLASS") {
                                var heartClassOK = ekgController.heartClassCompleted
                                if (!hasData) {
                                    return "Oczekiwanie na plik"
                                } else if (!hasFiltered) {
                                    return "Oczekiwanie na filtrowanie"
                                } else if (processing) {
                                    return "Przetwarzanie..."
                                } else if (!heartClassOK) {
                                    return "Gotowy"
                                } else {
                                    return "Sklasyfikowano uderzenia serca"
                                }
                            } else {
                                return hasData ? "Oczekiwanie na analize" : "Oczekiwanie na import"
                            }
                        }
                        text: statusText
                        property color statusColor: {
                            var processing = window.isProcessing
                            var module = window.currentModule
                            var hasData = ekgController.hasData
                            var hasFiltered = ekgController.hasFilteredData
                            var baselineOK = ekgController.baselineCompleted
                            var rPeaksOK = ekgController.rPeaksCompleted
                            var hrvTimeOK = ekgController.hrvTimeCompleted

                            if (module === "ECG BASELINE") {
                                if (!hasData) return textSecondary
                                if (processing) return Material.color(Material.Orange)
                                if (!baselineOK) return Material.color(Material.Teal)
                                return Material.color(Material.Green)
                            } else if (module === "R PEAKS") {
                                if (!hasData) return textSecondary
                                if (!hasFiltered) return textSecondary
                                if (processing) return Material.color(Material.Orange)
                                if (!rPeaksOK) return Material.color(Material.Teal)
                                return Material.color(Material.Green)
                            } else if (module === "HRV TIME") {
                                if (!hasData) return textSecondary
                                if (!rPeaksOK) return textSecondary
                                if (processing) return Material.color(Material.Orange)
                                if (!hrvTimeOK) return Material.color(Material.Teal)
                                return Material.color(Material.Green)
                            } else if (module === "HRV GEO") {
                                var hrvGeoOK = ekgController.hrvGeoCompleted
                                if (!hasData) return textSecondary
                                if (!rPeaksOK) return textSecondary
                                if (processing) return Material.color(Material.Orange)
                                if (!hrvGeoOK) return Material.color(Material.Teal)
                                return Material.color(Material.Green)
                            } else if (module === "WAVES") {
                                var wavesOK = ekgController.wavesCompleted
                                if (!hasData) return textSecondary
                                if (!hasFiltered) return textSecondary
                                if (processing) return Material.color(Material.Orange)
                                if (!wavesOK) return Material.color(Material.Teal)
                                return Material.color(Material.Green)
                            } else if (module === "HEART CLASS") {
                                var heartClassOK = ekgController.heartClassCompleted
                                if (!hasData) return textSecondary
                                if (!hasFiltered) return textSecondary
                                if (processing) return Material.color(Material.Orange)
                                if (!heartClassOK) return Material.color(Material.Teal)
                                return Material.color(Material.Green)
                            }
                            return textSecondary
                        }
                        color: statusColor
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Component.onCompleted: {
                            Qt.callLater(function () {
                            })
                        }
                    }

                    ProgressBar {
                        id: analysisProgress
                        from: 0
                        to: 100
                        value: window.isProcessing ? 0 : 100
                        indeterminate: window.isProcessing
                        Layout.fillWidth: true
                    }
                }

                Loader {
                    id: paramsLoader
                    Layout.fillWidth: true
                    sourceComponent:
                            window.currentModule === "ECG BASELINE" ? baselineParams :
                            window.currentModule === "R PEAKS" ? rPeaksParams :
                                window.currentModule === "WAVES" ? wavesParams :
                                    window.currentModule === "HRV TIME" ? hrvTimeParams :
                                        window.currentModule === "HRV GEO" ? hrvGeoParams :
                                            window.currentModule === "HEART CLASS" ? heartClassParams :
                                            null
                    onLoaded: {
                        if (window.loadedSettings) {
                            Qt.callLater(applyLoadedSettings)
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                    Layout.minimumHeight: 0
                    opacity: window.currentModule !== "HRV GEO" ? 1 : 0
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
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
                            } else if (window.currentModule === "HEART CLASS") {
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
                            } else if (window.currentModule === "HEART CLASS" && !ekgController.hasFilteredData) {
                                return "Najpierw uruchom filtrowanie baseline"
                            }
                            return ""
                        }
                        ToolTip.delay: 500

                        onClicked: {
                            window.isProcessing = true
                            analysisDelayTimer.start()
                        }
                        
                        Timer {
                            id: analysisDelayTimer
                            interval: 50
                            repeat: false
                            onTriggered: {
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
                                } else if (window.currentModule === "HEART CLASS") {
                                    if (paramsLoader.item && paramsLoader.item.runHeartClass) {
                                        paramsLoader.item.runHeartClass()
                                    }
                                }
                            }
                        }
                    }


                    Button {
                        text: "Reset"
                        Layout.preferredWidth: 100
                        onClicked: {
                            window.isProcessing = false
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
                            } else if (window.currentModule === "HEART CLASS") {
                                ekgController.resetHeartClass()
                                heartClassRan = false
                                heartClassAnnotations = []
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
    }

    Component {
        id: baselineParams

        ColumnLayout {
            id: baselineRoot
            Layout.fillWidth: true
            spacing: 8

            property int windowSize: 5
            property int polynomialOrder: 2
            property alias rbMovingAverage: rbMovingAverage
            property alias rbButterworth: rbButterworth
            property alias rbSavitzkyGolay: rbSavitzkyGolay

            Component.onCompleted: {
                if (window.selectedFilterMethod === 0) rbMovingAverage.checked = true
                else if (window.selectedFilterMethod === 1) rbButterworth.checked = true
                else if (window.selectedFilterMethod === 2) rbSavitzkyGolay.checked = true
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
                    window.isProcessing = false
                    showTemporaryStatus("⚠ Wybierz filtr", Material.Orange)
                    return
                }

                chartLoading = true

                if (rbMovingAverage.checked) {
                    window.selectedFilterMethod = 0
                    ekgController.runBaseline(0, windowSize, polynomialOrder)
                } else if (rbButterworth.checked) {
                    window.selectedFilterMethod = 1
                    ekgController.runBaseline(1, windowSize, polynomialOrder)
                } else if (rbSavitzkyGolay.checked) {
                    window.selectedFilterMethod = 2
                    ekgController.runBaseline(2, windowSize, polynomialOrder)
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
                leftPadding: 16
                onCheckedChanged: if (checked) window.selectedFilterMethod = 0
            }

            RadioButton {
                id: rbButterworth
                text: "Butterworth"
                ButtonGroup.group: filterGroup
                leftPadding: 16
                onCheckedChanged: if (checked) window.selectedFilterMethod = 1
            }

            RadioButton {
                id: rbSavitzkyGolay
                text: "Savitzky-Golay"
                ButtonGroup.group: filterGroup
                leftPadding: 16
                onCheckedChanged: if (checked) window.selectedFilterMethod = 2
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: borderColor
                opacity: rbMovingAverage.checked || rbSavitzkyGolay.checked ? 1 : 0
                Layout.topMargin: 8
                Layout.bottomMargin: 8
            }

            RowLayout {
                Layout.fillWidth: true
                visible: rbMovingAverage.checked || rbSavitzkyGolay.checked
                spacing: 8

                Label {
                    text: "Rozmiar okna:"
                    color: textSecondary
                    font.pixelSize: 13
                }

                SpinBox {
                    id: windowSizeSpinBox
                    from: 3
                    to: 51
                    stepSize: 2
                    value: baselineRoot.windowSize
                    onValueChanged: {
                        var val = value
                        if (val % 2 === 0) val++
                        baselineRoot.windowSize = val
                    }
                    editable: true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: rbSavitzkyGolay.checked
                spacing: 8

                Label {
                    text: "Stopień wielomianu:"
                    color: textSecondary
                    font.pixelSize: 13
                }

                SpinBox {
                    id: polynomialOrderSpinBox
                    from: 1
                    to: 10
                    value: baselineRoot.polynomialOrder
                    onValueChanged: baselineRoot.polynomialOrder = value
                    editable: true
                }
            }
        }
    }

    Component {
        id: rPeaksParams

        ColumnLayout {
            id: rPeaksRoot
            Layout.fillWidth: true
            spacing: 8

            property alias rbPanTompkins: rbPanTompkins
            property alias rbHilbert: rbHilbert
            property alias rbWavelet: rbWavelet

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
                    window.isProcessing = false
                    showTemporaryStatus("⚠ Wybierz metode detekcji", Material.Orange)
                    return
                }

                chartLoading = true

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
                text: "Wybierz metodę detekcji:"
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
                leftPadding: 16
                onCheckedChanged: if (checked) window.selectedRPeaksMethod = 0
            }

            RadioButton {
                id: rbHilbert
                text: "Transformata Hilberta"
                ButtonGroup.group: detectionMethodGroup
                leftPadding: 16
                onCheckedChanged: if (checked) window.selectedRPeaksMethod = 1
            }

            RadioButton {
                id: rbWavelet
                text: "Falkowa (Wavelet)"
                ButtonGroup.group: detectionMethodGroup
                leftPadding: 16
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

            property alias rbClassicPeriodogram: rbClassicPeriodogram
            property alias rbLombScargle: rbLombScargle
            property alias rbWelch: rbWelch

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
                    window.isProcessing = false
                    showTemporaryStatus("⚠ Wybierz metode estymacji", Material.Orange)
                    return
                }

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
                leftPadding: 16
                onCheckedChanged: if (checked) window.selectedHRVTimeMethod = 0
            }

            RadioButton {
                id: rbLombScargle
                text: "Lomb-Scargle"
                ButtonGroup.group: spectralMethodGroup
                leftPadding: 16
                onCheckedChanged: if (checked) window.selectedHRVTimeMethod = 1
            }

            RadioButton {
                id: rbWelch
                text: "Welch"
                ButtonGroup.group: spectralMethodGroup
                leftPadding: 16
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

                Label {
                    text: "RR mean:"
                }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().rr_mean.toFixed(2) + " ms" : "-"
                    color: Material.color(Material.Teal)
                }

                Label {
                    text: "SDNN:"
                }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().sdnn.toFixed(2) + " ms" : "-"
                    color: Material.color(Material.Teal)
                }

                Label {
                    text: "RMSSD:"
                }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().rmssd.toFixed(2) + " ms" : "-"
                    color: Material.color(Material.Teal)
                }

                Label {
                    text: "NN50:"
                }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().nn50 : "-"
                    color: Material.color(Material.Teal)
                }

                Label {
                    text: "PNN50:"
                }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().pnn50.toFixed(2) + " %" : "-"
                    color: Material.color(Material.Teal)
                }

                Label {
                    text: "Metryki częstotliwościowe:"
                    font.bold: true
                    font.pixelSize: 14
                    Layout.columnSpan: 2
                    Layout.topMargin: 8
                }

                Label {
                    text: "Total Power:"
                }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().tp.toFixed(2) + " ms²" : "-"
                    color: Material.color(Material.Teal)
                }

                Label {
                    text: "VLF:"
                }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().vlf.toFixed(2) + " ms²" : "-"
                    color: Material.color(Material.Teal)
                }

                Label {
                    text: "LF:"
                }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().lf.toFixed(2) + " ms²" : "-"
                    color: Material.color(Material.Teal)
                }

                Label {
                    text: "HF:"
                }
                Label {
                    text: ekgController.hrvTimeCompleted ? ekgController.getHRVTimeMetrics().hf.toFixed(2) + " ms²" : "-"
                    color: Material.color(Material.Teal)
                }

                Label {
                    text: "LF/HF:"
                }
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
                ekgController.runHRVGeo()
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: borderColor
                opacity: ekgController.hrvGeoCompleted ? 1 : 0
                Layout.topMargin: 8
                Layout.bottomMargin: 8
            }

            GridLayout {
                visible: window.currentModule === "HRV GEO"
                columns: 2
                columnSpacing: 12
                rowSpacing: 6
                Layout.fillWidth: true

                Label {
                    text: "Metryki trójkątne:"
                    font.bold: true
                    font.pixelSize: 14
                    Layout.columnSpan: 2
                }

                Label {
                    text: "Triangular Index:"
                }
                Label {
                    text: ekgController.hrvGeoCompleted ? ekgController.getHRVGeoMetrics().triangular_index.toFixed(2) : "-"
                    color: ekgController.hrvGeoCompleted ? Material.color(Material.Teal) : textSecondary
                }

                Label {
                    text: "TINN:"
                }
                Label {
                    text: ekgController.hrvGeoCompleted ? ekgController.getHRVGeoMetrics().tinn.toFixed(2) + " ms" : "-"
                    color: ekgController.hrvGeoCompleted ? Material.color(Material.Teal) : textSecondary
                }

                Label {
                    text: "Metryki Poincaré:"
                    font.bold: true
                    font.pixelSize: 14
                    Layout.columnSpan: 2
                    Layout.topMargin: 8
                }

                Label {
                    text: "SD1:"
                }
                Label {
                    text: ekgController.hrvGeoCompleted ? ekgController.getHRVGeoMetrics().sd1.toFixed(2) + " ms" : "-"
                    color: ekgController.hrvGeoCompleted ? Material.color(Material.Teal) : textSecondary
                }

                Label {
                    text: "SD2:"
                }
                Label {
                    text: ekgController.hrvGeoCompleted ? ekgController.getHRVGeoMetrics().sd2.toFixed(2) + " ms" : "-"
                    color: ekgController.hrvGeoCompleted ? Material.color(Material.Teal) : textSecondary
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
                chartLoading = true
                ekgController.runWaves()
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: borderColor
                opacity: ekgController.wavesCompleted ? 1 : 0
                Layout.topMargin: 8
                Layout.bottomMargin: 8
            }

            GridLayout {
                visible: window.currentModule === "WAVES"
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

                Label {
                    text: "P onset:"
                }
                Label {
                    text: ekgController.wavesCompleted && chartWaveMarkers.p_onsets ? chartWaveMarkers.p_onsets.length : "-"
                    color: ekgController.wavesCompleted ? Material.color(Material.Teal) : textSecondary
                }

                Label {
                    text: "P end:"
                }
                Label {
                    text: ekgController.wavesCompleted && chartWaveMarkers.p_ends ? chartWaveMarkers.p_ends.length : "-"
                    color: ekgController.wavesCompleted ? Material.color(Material.Teal) : textSecondary
                }

                Label {
                    text: "QRS onset:"
                }
                Label {
                    text: ekgController.wavesCompleted && chartWaveMarkers.qrs_onsets ? chartWaveMarkers.qrs_onsets.length : "-"
                    color: ekgController.wavesCompleted ? Material.color(Material.Teal) : textSecondary
                }

                Label {
                    text: "QRS end:"
                }
                Label {
                    text: ekgController.wavesCompleted && chartWaveMarkers.qrs_ends ? chartWaveMarkers.qrs_ends.length : "-"
                    color: ekgController.wavesCompleted ? Material.color(Material.Teal) : textSecondary
                }

                Label {
                    text: "T end:"
                }
                Label {
                    text: ekgController.wavesCompleted && chartWaveMarkers.t_ends ? chartWaveMarkers.t_ends.length : "-"
                    color: ekgController.wavesCompleted ? Material.color(Material.Teal) : textSecondary
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: borderColor
                opacity: ekgController.wavesCompleted ? 1 : 0
                Layout.topMargin: 8
                Layout.bottomMargin: 8
            }

            ColumnLayout {
                visible: window.currentModule === "WAVES" && ekgController.wavesCompleted
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: "Legenda:"
                    font.bold: true
                    font.pixelSize: 14
                }

                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: "#f59e0b"
                    }
                    Label {
                        text: "P onset"
                        font.pixelSize: 12
                    }
                }

                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: "#eab308"
                    }
                    Label {
                        text: "P end"
                        font.pixelSize: 12
                    }
                }

                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: "#8b5cf6"
                    }
                    Label {
                        text: "QRS onset"
                        font.pixelSize: 12
                    }
                }

                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: "#a78bfa"
                    }
                    Label {
                        text: "QRS end"
                        font.pixelSize: 12
                    }
                }

                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: "#ec4899"
                    }
                    Label {
                        text: "T end"
                        font.pixelSize: 12
                    }
                }
            }
        }
    }

    Component {
        id: heartClassParams

        ColumnLayout {
            id: heartClassRoot
            Layout.fillWidth: true
            spacing: 8

            function isReady() {
                return ekgController.hasFilteredData
            }

            function resetState() {
            }

            function runHeartClass() {
                ekgController.runHeartClass()
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: borderColor
                visible: ekgController.heartClassCompleted
                Layout.topMargin: 8
                Layout.bottomMargin: 8
            }

            GridLayout {
                visible: ekgController.heartClassCompleted
                columns: 2
                columnSpacing: 12
                rowSpacing: 6
                Layout.fillWidth: true

                Label {
                    text: "Statystyki klasyfikacji:"
                    font.bold: true
                    font.pixelSize: 14
                    Layout.columnSpan: 2
                }

                Label {
                    text: "Normalne (N):"
                }
                Label {
                    text: {
                        if (!ekgController.heartClassCompleted) return "-"
                        var barData = ekgController.getHeartClassBarChart()
                        return barData.N + " (" + barData.percentN.toFixed(1) + "%)"
                    }
                    color: Material.color(Material.Green)
                }

                Label {
                    text: "Komorowe (V):"
                }
                Label {
                    text: {
                        if (!ekgController.heartClassCompleted) return "-"
                        var barData = ekgController.getHeartClassBarChart()
                        return barData.V + " (" + barData.percentV.toFixed(1) + "%)"
                    }
                    color: Material.color(Material.Red)
                }

                Label {
                    text: "Przedsionkowe (A):"
                }
                Label {
                    text: {
                        if (!ekgController.heartClassCompleted) return "-"
                        var barData = ekgController.getHeartClassBarChart()
                        return barData.A + " (" + barData.percentA.toFixed(1) + "%)"
                    }
                    color: Material.color(Material.Orange)
                }

                Label {
                    text: "Inne:"
                }
                Label {
                    text: {
                        if (!ekgController.heartClassCompleted) return "-"
                        var barData = ekgController.getHeartClassBarChart()
                        return barData.Other + " (" + barData.percentOther.toFixed(1) + "%)"
                    }
                    color: Material.color(Material.Grey)
                }

                Label {
                    text: "Razem:"
                }
                Label {
                    text: {
                        if (!ekgController.heartClassCompleted) return "-"
                        var barData = ekgController.getHeartClassBarChart()
                        return barData.total
                    }
                    color: Material.color(Material.Teal)
                    font.bold: true
                }
            }

            ColumnLayout {
                visible: ekgController.heartClassCompleted && heartClassAnnotations.length > 0
                Layout.fillWidth: true
                spacing: 4
                Layout.topMargin: 8

                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 12; height: 12; radius: 6; color: "#22c55e"
                    }
                    Label {
                        text: "N - Normalne"; font.pixelSize: 12
                    }
                }

                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 12; height: 12; radius: 6; color: "#ef4444"
                    }
                    Label {
                        text: "V - Komorowe (PVC)"; font.pixelSize: 12
                    }
                }

                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 12; height: 12; radius: 6; color: "#f59e0b"
                    }
                    Label {
                        text: "A - Przedsionkowe (PAC)"; font.pixelSize: 12
                    }
                }
            }

            ListView {
                id: heartClassList
                visible: ekgController.heartClassCompleted && heartClassAnnotations.length > 0
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(200, heartClassAnnotations.length * 30)
                clip: true
                model: heartClassAnnotations

                delegate: RowLayout {
                    width: ListView.view.width
                    spacing: 8

                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: {
                            var label = modelData.label
                            if (label === "N") return "#22c55e"
                            if (label === "V") return "#ef4444"
                            if (label === "A") return "#f59e0b"
                            return "#9ca3af"
                        }
                    }

                    Label {
                        text: modelData.time.toFixed(2) + "s"
                        font.pixelSize: 12
                        Layout.preferredWidth: 60
                    }

                    Label {
                        text: modelData.label
                        font.bold: true
                        font.pixelSize: 12
                        color: {
                            var label = modelData.label
                            if (label === "N") return "#22c55e"
                            if (label === "V") return "#ef4444"
                            if (label === "A") return "#f59e0b"
                            return textSecondary
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }
        }
    }

    Dialog {
        id: helpDialog
        title: "O programie"
        modal: true
        standardButtons: Dialog.Ok
        implicitWidth: 420

        onVisibleChanged: if (visible) {
            x = (window.width - implicitWidth) / 2
            y = (window.height - implicitHeight) / 2
        }

        contentItem: ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 8

            Label {
                leftPadding: 16
                text: "Projekt Dedykowane Algorytmy Diagnostyki Medycznej - Analiza sygnału EKG 2026\n\n" +
                    "IB IEM AGH\n\n" +
                    "Autorzy:\n" +
                    "Project Manager - Wiktor Raczek\n" +
                    "Software Architect - Mateusz Woźniak\n" +
                    "GUI - Oliwia Rewer\n" +
                    "Wizualizacja - Sonia Stanula\n" +
                    "IO - Paulina Wór\n" +
                    "Baseline - Aleksandra Szota\n" +
                    "R Peaks - Mateusz Piotrowski\n" +
                    "Waves - Magdalena Suchan\n" +
                    "HRV Time - Jakub Nowak\n" +
                    "HRV Geo - Jakub Kalina\n" +
                    "HRV DFA - Hubert Piechura\n" +
                    "Heart Class - Jeremiasz Potoczny"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

    FileDialog {
        id: exportFileDialog
        title: "Zapisz wyniki"
        fileMode: FileDialog.SaveFile
        nameFilters: [
            "CSV files (*.csv)",
            "HTML files (*.html)",
            "JSON files (*.json)",
            "All files (*)"
        ]
        property string selectedFormat: "csv"

        function getFormatEnum(formatString) {
            if (formatString === "csv") return 0
            if (formatString === "html") return 1
            if (formatString === "json") return 2
            return 0
        }

        onAccepted: {
            var filepath = currentFile.toString()
            if (filepath.startsWith("file://")) {
                filepath = filepath.substring(7)
            }
            var success = false
            var formatEnum = getFormatEnum(exportFileDialog.selectedFormat)

            if (window.currentModule === "ECG BASELINE") {
                success = ekgController.exportFilteredSignal(formatEnum, filepath)
            } else if (window.currentModule === "R PEAKS") {
                success = ekgController.exportRPeaks(formatEnum, filepath)
            } else if (window.currentModule === "HRV TIME") {
                success = ekgController.exportHRVTime(formatEnum, filepath)
            } else if (window.currentModule === "HRV GEO") {
                success = ekgController.exportHRVGeo(formatEnum, filepath)
            } else if (window.currentModule === "WAVES") {
                success = ekgController.exportWaves(formatEnum, filepath)
            } else if (window.currentModule === "HEART CLASS") {
                success = ekgController.exportHeartClass(formatEnum, filepath)
            }

            if (success) {
                exportSuccessDialog.open()
            } else {
                exportErrorDialog.open()
            }
        }
    }

    Dialog {
        id: exportSuccessDialog
        title: "Sukces"
        modal: true
        standardButtons: Dialog.Ok
        implicitWidth: 300

        onVisibleChanged: if (visible) {
            x = (window.width - implicitWidth) / 2
            y = (window.height - implicitHeight) / 2
        }

        contentItem: Label {
            text: "Wyniki zostały zapisane pomyślnie."
            wrapMode: Text.WordWrap
        }
    }

    Dialog {
        id: exportErrorDialog
        title: "Błąd"
        modal: true
        standardButtons: Dialog.Ok
        implicitWidth: 300

        onVisibleChanged: if (visible) {
            x = (window.width - implicitWidth) / 2
            y = (window.height - implicitHeight) / 2
        }

        contentItem: Label {
            text: "Nie udało się zapisać wyników. Upewnij się, że moduł został uruchomiony i że masz uprawnienia do zapisu pliku."
            wrapMode: Text.WordWrap
        }
    }

    Dialog {
        id: loadConfirmDialog
        title: "Potwierdzenie"
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        implicitWidth: 400

        onVisibleChanged: if (visible) {
            x = (window.width - implicitWidth) / 2
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
                text: "Czy na pewno chcesz załadować plik " + pendingFileName + "?"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
}

