#include "storage.h"

Gekko::StateStorage::StateStorage()
{
	_max_num_states = 0;
}


void Gekko::StateStorage::Init(u32 num_states, u32 state_size, bool limited)
{
	// Confirmed-plus-speculative saving needs stable slots throughout the
	// prediction window. Gekko states are tiny embedding-owned handles in
	// HorseMod, so retaining the normal ring is both bounded and inexpensive.
	const u32 num = num_states + 2;
	(void)limited;
	_max_num_states = num;

	for (u32 i = 0; i < _max_num_states; i++) {
		_states.push_back(std::make_unique<StateEntry>());
        _states.back().get()->state = std::make_unique<u8[]>(state_size);
		_states.back().get()->state_len = state_size;
	}

	_runahead_state.state = std::make_unique<u8[]>(state_size);
	_runahead_state.state_len = state_size;
	_runahead_state.frame = GameInput::NULL_FRAME;
}

Gekko::StateEntry* Gekko::StateStorage::GetRunaheadState()
{
	return &_runahead_state;
}

Gekko::StateEntry* Gekko::StateStorage::GetState(Frame frame)
{
	frame = frame < 0 ? frame + _max_num_states : frame;
	return _states[frame % _max_num_states].get();
}

Gekko::StateEntry* Gekko::StateStorage::FindLatestAtOrBefore(Frame frame)
{
    StateEntry* latest = nullptr;
    for (auto& candidate : _states) {
        if (candidate->frame <= frame
            && candidate->frame != GameInput::NULL_FRAME
            && (!latest || candidate->frame > latest->frame)) {
            latest = candidate.get();
        }
    }
    return latest;
}

Gekko::StateEntry* Gekko::StateStorage::FindLatestBefore(Frame frame)
{
    return FindLatestAtOrBefore(frame - 1);
}

void Gekko::StateStorage::InvalidateFrom(Frame frame)
{
    for (auto& candidate : _states) {
        if (candidate->frame >= frame) {
            candidate->frame = GameInput::NULL_FRAME;
            candidate->checksum = 0;
        }
    }
}
