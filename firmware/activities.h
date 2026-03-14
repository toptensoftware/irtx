#pragma once

#include "activities_types.h"

extern activitiesRoot* activitiesConfig;

void setupActivities();
void pollActivities();
bool reloadActivities();
