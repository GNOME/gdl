INCLUDES= `gtk-config --cflags` -DGTK -DSCI_LEXER -W -Wall -I .

noinst_LIBRARIES=libscintilla-widget.a libthrowaway.a

libthrowaway_a_SOURCES = \
	LexCPP.cxx       \
	LexHTML.cxx      \
	LexLua.cxx       \
	LexOthers.cxx    \
	LexPerl.cxx      \
	LexPython.cxx    \
	LexSQL.cxx       \
	LexVB.cxx

libscintilla_widget_a_SOURCES= \
	DocumentAccessor.cxx \
	WindowAccessor.cxx \
	KeyWords.cxx \
	Document.cxx \
	CallTip.cxx \
	ScintillaBase.cxx \
	ContractionState.cxx \
	Editor.cxx \
	PropSet.cxx \
	PlatGTK.cxx \
	KeyMap.cxx \
	LineMarker.cxx \
	ScintillaGTK.cxx \
	CellBuffer.cxx \
	ViewStyle.cxx \
	Style.cxx \
	Indicator.cxx \
	AutoComplete.cxx


