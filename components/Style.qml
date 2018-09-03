pragma Singleton

import QtQuick 2.5

QtObject {
    property QtObject fontMedium: FontLoader { id: _fontMedium; source: "qrc:/fonts/SFUIDisplay-Medium.otf"; }
    property QtObject fontBold: FontLoader { id: _fontBold; source: "qrc:/fonts/SFUIDisplay-Bold.otf"; }
    property QtObject fontLight: FontLoader { id: _fontLight; source: "qrc:/fonts/SFUIDisplay-Light.otf"; }
    property QtObject fontRegular: FontLoader { id: _fontRegular; source: "qrc:/fonts/SFUIDisplay-Regular.otf"; }

    property string grey: "#A8A8A8"

    property string defaultFontColor: "white"
    property string greyFontColor: "#C9C9C9"
    property string dimmedFontColor: "#BBBBBB"
    property string inputBoxBackground: "black"
    property string inputBoxBackgroundError: "#FFDDDD"
    property string inputBoxColor: "white"
    property string legacy_placeholderFontColor: "#BABABA"

    property string buttonBackgroundColor: "#BC2F42"
    property string buttonBackgroundColorHover: "#2E1866"
    property string buttonBackgroundColorDisabled: "#707070"
    property string buttonBackgroundColorDisabledHover: "#808080"
    property string buttonTextColor: "white"
    property string buttonTextColorDisabled: "white"
    property string dividerColor: "darkred"
    property real dividerOpacity: 0.25
}
