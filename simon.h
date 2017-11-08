#include <cstdlib>
#include <iostream>
#include <vector>
#include <ctime>
#include <sys/time.h>

using namespace std;

class Simon {

public:
	// possible game states:
	// PAUSE - nothing happening
	// COMPUTER - computer is play a sequence of buttons
	// HUMAN - human is guessing the sequence of buttons
	// LOSE and WIN - game is over in one of thise states
	enum State { START, COMPUTER, HUMAN, LOSE, WIN };

protected:

	// the game state and score
	State state;
	int score;

	// length of sequence
	int length;
	// number of possible buttons
	int buttons;

	// the sequence of buttons and current button
	vector<int> sequence;
	int index;

	bool debug;

	void init(int _buttons, bool _debug){

		// true will output additional information
		// (you will want to turn this off before)
		debug = _debug;

		length = 1;
		buttons = _buttons;
		state = START;
		score = 0;

		if (debug) { cout << "[DEBUG] starting " << buttons << " button game" << endl; }
	}

public:

	Simon(int _buttons) { init(_buttons, false); }

	Simon(int _buttons, bool _debug) { init(_buttons, _debug); }

    // get microseconds
    unsigned long now() {
        timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000000 + tv.tv_usec;
    }

	int getNumButtons() { return buttons; }

	int getScore() { return score; }

	State getState() { return state; }

	string getStateAsString() {

		switch (getState()) {

		case Simon::START:
			return "START";
			break;

		case Simon::COMPUTER:
			return "COMPUTER";
			break;

		case Simon::HUMAN:
			return "HUMAN";
			break;

		case Simon::LOSE:
			return "LOSE";
			break;

		case Simon::WIN:
			return "WIN";
			break;
		default:
			return "Unkown State";
			break;
		}
	}

	void newRound() {

		if (debug) {
			cout << "[DEBUG] newRound, Simon::state "
			     << getStateAsString() << endl;
		}

		// reset if they lost last time
		if (state == LOSE) {
			if (debug) { cout << "[DEBUG] reset length and score after loss" << endl; }
			length = 1;
			score = 0;
		}

		sequence.clear();

		if (debug) { cout << "[DEBUG] new sequence: "; }

		for (int i = 0; i < length; i++) {
            srand(now());
			int b = 0;
            b = rand() % buttons;
            if (b == 0){
                b = buttons;
            };
			sequence.push_back(b);
			if (debug) { cout << b << " "; }
		}
		if (debug) { cout << endl; }

		index = 0;
		state = COMPUTER;

	}

	// call this to get next button to show when computer is playing
	int nextButton() {

		if (state != COMPUTER) {
			cout << "[WARNING] nextButton called in "
			     << getStateAsString() << endl;
			return -1;
		}

		// this is the next button to show in the sequence
		int button = sequence[index];

		if (debug) {
			cout << "[DEBUG] nextButton:  index " << index
			     << " button " << button
			     << endl;
		}

		// advance to next button
		index++;

		// if all the buttons were shown, give
		// the human a chance to guess the sequence
		if (index >= sequence.size()) {
			index = 0;
			state = HUMAN;
		}

		return button;
	}

	bool verifyButton(int button) {

		if (state != HUMAN) {
			cout << "[WARNING] verifyButton called in "
			     << getStateAsString() << endl;
			return false;
		}

		// did they press the right button?
		bool correct = (button == sequence[index]);

		if (debug) {
			cout << "[DEBUG] verifyButton: index " << index
			     << ", pushed " << button
			     << ", sequence " << sequence[index];
		}

		// advance to next button
		index++;

		// pushed the wrong buttons
		if (!correct) {
			state = LOSE;
			if (debug) {
				cout << ", wrong. " << endl;
				cout << "[DEBUG] state is now " << getStateAsString() << endl;
			}

			// they got it right
		} else {
			if (debug) { cout << ", correct." << endl; }

			// if last button, then the win the round
			if (index == sequence.size()) {
				state = WIN;
				// update the score and increase the difficulty
				score++;
				length++;

				if (debug) {
					cout << "[DEBUG] state is now " << getStateAsString() << endl;
					cout << "[DEBUG] new score " << score
					     << ", length increased to " << length
					     << endl;
				}
			}
		}
		return correct;
	}

};

