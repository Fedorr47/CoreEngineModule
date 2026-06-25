#pragma once

import core;

struct InlineThreadOwnerRolesGuard
{
    InlineThreadOwnerRolesGuard()
    {
        threadAffinity::ResetOwnerThreadRegistry();
        threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Main);
        threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Runtime);
        threadAffinity::RegisterOwnerThread(threadAffinity::ThreadOwnerRole::Render);
    }

    ~InlineThreadOwnerRolesGuard()
    {
        threadAffinity::ResetOwnerThreadRegistry();
    }

    InlineThreadOwnerRolesGuard(const InlineThreadOwnerRolesGuard&) = delete;
    InlineThreadOwnerRolesGuard& operator=(const InlineThreadOwnerRolesGuard&) = delete;
};
