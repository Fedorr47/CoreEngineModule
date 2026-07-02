#pragma once

#ifndef NDEBUG
#define CORE_ASSERT_OWNER_THREAD(role) ::threadAffinity::AssertOwnerThread(role)
#define CORE_ASSERT_MAIN_THREAD() CORE_ASSERT_OWNER_THREAD(::threadAffinity::ThreadOwnerRole::Main)
#define CORE_ASSERT_RUNTIME_THREAD() CORE_ASSERT_OWNER_THREAD(::threadAffinity::ThreadOwnerRole::Runtime)
#define CORE_ASSERT_RENDER_THREAD() CORE_ASSERT_OWNER_THREAD(::threadAffinity::ThreadOwnerRole::Render)
#else
#define CORE_ASSERT_OWNER_THREAD(role) do { } while (false)
#define CORE_ASSERT_MAIN_THREAD() do { } while (false)
#define CORE_ASSERT_RUNTIME_THREAD() do { } while (false)
#define CORE_ASSERT_RENDER_THREAD() do { } while (false)
#endif