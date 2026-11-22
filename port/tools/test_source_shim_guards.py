"""Check how the SDL shim source scan treats compile-time branches."""

import unittest

from check_sources import guest_constant_names, shim_problems


class GuestConstantsTest(unittest.TestCase):
    def test_current_sources(self):
        self.assertEqual([], shim_problems())

    def test_native_branch_and_nested_conditions_are_excluded(self):
        source = """
#ifndef PORT_SDL_BRIDGE
# ifdef VERSION_EU
SDL_EVENT_NATIVE_EU
# else
SDL_EVENT_NATIVE_OTHER
# endif
#elif defined(PORT_SDL_BRIDGE)
SDL_EVENT_CONSOLE
#else
SDL_EVENT_IMPOSSIBLE
#endif
SDL_EVENT_SHARED
"""
        self.assertEqual({"SDL_EVENT_CONSOLE", "SDL_EVENT_SHARED"}, guest_constant_names(source))

    def test_unknown_branch_does_not_hide_other_targets(self):
        source = """
#if defined(PORT_SWITCH)
SDL_EVENT_SWITCH
#elif defined(PORT_SDL_BRIDGE)
SDL_EVENT_OTHER_GUEST
#else
SDL_EVENT_IMPOSSIBLE
#endif
#if !defined(PORT_SDL_BRIDGE) /* native APIs */
SDL_EVENT_NATIVE
#else
SDL_EVENT_GUEST
#endif
"""
        self.assertEqual(
            {"SDL_EVENT_SWITCH", "SDL_EVENT_OTHER_GUEST", "SDL_EVENT_GUEST"}, guest_constant_names(source)
        )

    def test_comment_is_not_a_preprocessor_guard(self):
        source = """
/*
#ifndef PORT_SDL_BRIDGE
*/
SDL_EVENT_GUEST
"""
        self.assertEqual({"SDL_EVENT_GUEST"}, guest_constant_names(source))


if __name__ == "__main__":
    unittest.main()
