#include "pch.h"
#include "font_selection.h"

#include "options.h"
#include "pb.h"
#include "score.h"
#include "winmain.h"
#include "translations.h"

bool font_selection::ShowDialogFlag = false;
char font_selection::DialogInputBuffer[512];

void font_selection::ShowDialog()
{
	ShowDialogFlag = true;
}

void font_selection::RenderDialog()
{
	ShowDialogFlag = false;
}
