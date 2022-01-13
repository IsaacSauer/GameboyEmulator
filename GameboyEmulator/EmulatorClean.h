#pragma once
#include <string>
#include <bitset>

/**
 * \brief GameBoy Emulation Environment
 */
namespace gbee
{
	enum Key : uint8_t { right, aButton, left, bButton, up, select, down, start };
	
	/**
	 * \brief This class handles the entire emulation environment
	 */
	class Emulator
	{
	public:
		/**
		 * \param gbfile The ROM file to be read
		 * \param instances The amount of instances that will be spawned
		 */
		Emulator( const std::string &gbfile, const uint8_t instances );

		//lof
		Emulator( const Emulator &rhs ) = delete;
		Emulator( Emulator &&lhs ) = delete;
		Emulator& operator=( const Emulator &rhs ) = delete;
		Emulator& operator=( Emulator &&lhs ) = delete;
		
		/**
		 * \brief Loads the game and resets the gameboy instance
		 * \param gbFile The ROM file to be read
		 */
		void LoadGame( const std::string &gbFile) const;

		/**
		 * \brief Starts the Gameboy instances
		 */
		void Start() const;

		/**
		 * \brief Resets the gameboy
		 */
		void Reset() const;

		void TestCPU() const;
		
		/**
		 * \brief Returns the frame buffer for the requested instance
		 * \param instanceID The gameboy instance
		 * \return The Framebuffer
		 * \note Every pixel is a 2 bit value with \c 0 being white and \c 3 being black
		 */
		std::bitset<160 * 144 * 2> GetFrameBuffer( const uint8_t instanceID ) const;
		/**
		 * \brief Sets the key state for a Gameboy Instance
		 * \param key The key who's state needs to be set.<b> Use the Key enum!</b>
		 * \param state The state, \c true being pressed, \c false being released
		 * \param instanceID The Gameboy instance ID to use
		 */
		void SetKeyState( const uint8_t key, const bool state, const uint8_t instanceID ) const;
		/**
		 * \brief Lets the Gameboy instance run for \c cycles amount of cycles and then pauses indefinitely
		 * \param cycles The amount of cycles for which the instance should run
		 * \param instanceID The Gameboy Instance ID
		 */
		void RunForCycles( const unsigned short cycles, const uint8_t instanceID ) const;
		/**
		 * \brief Lets the Gameboy instance run for \c frames amount of frames and then pauses indefinitely
		 * \param frames The amount of frames for which the instance should run
		 * \param onlyDrawLastFrame Whether or not to skip drawing until the last frame
		 * \param instanceID The Gameboy Instance ID
		 */
		void RunForFrames( const unsigned short frames, const bool onlyDrawLastFrame, const uint8_t instanceID ) const;
		
		/**
		 * \brief Sets the pause state for a Gameboy Instance
		 * \param state The pause state, \c true being paused \c false being un-paused
		 * \param instanceID The Gameboy instance ID
		 */
		void SetPauseState( const bool state, const uint8_t instanceID ) const;
		/**
		 * \brief Returns the current pause state of a Gameboy instance
		 * \param instanceID The Gameboy Instance ID
		 * \return The current pause state of the requested instance
		 */
		bool GetPauseState( const uint8_t instanceID ) const;

		/**
		 * \brief Modifies the amount of cycles that are processed at once
		 * \param cycleMultiplier The multiplier to be applied to the amount of cycles processed
		 * \param instanceID The instance ID
		 */
		void SetSpeed( const uint16_t cycleMultiplier, const uint8_t instanceID ) const;
		uint16_t GetSpeed( const uint8_t instanceID ) const;
		/**
		 * \brief Changes the behavior of the automatic speed adjuster\n When enabled, the highest possible speed will be maintained
		 * \param onOff Whether or not to set the AutoSpeed
		 * \param instanceID The instance ID
		 * \note In the end, no real benefit is gained from this. Setting the speed crazy high and the auto adjuster disabled will also guarantee the highest speed
		 */
		void SetAutoSpeed( const bool onOff, const uint8_t instanceID ) const;
	};
}
