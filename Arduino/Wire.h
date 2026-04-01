class TwoWire {
public:
    void begin(int sda = -1, int scl = -1) {
    }

    void beginTransmission(int address) {
    }

    int endTransmission() {
        return 0;
    }
};

inline TwoWire Wire;
