#pragma once

// The static data manager of the BSMain library: InitSDM runs the other BSMain classes' tag setup (BSAttachPoints,
// BSBound, BSXFlags ...) once. Everything in it is static.
class BSMain {
public:
	static void InitSDM();
	static void KillSDM();
};
