void saveAudioInferences(uint8_t numberOfClassifiers, uint32_t sofMaxStartingAddress);
void savePosteriorFilterDecision(int classifier, int currentFile);
void saveAudioData(uint8_t classifier, int extrStides, int currentFile, uint32_t tankAddress, uint32_t tankSize);
void saveLongAudioData(int dataSavePeriodInSeconds, int currentFile, uint32_t tankAddress, uint32_t tankSize);
void saveLongSensorData(int sensorAxes, int dataSavePeriodInSeconds, int currentFile, uint32_t tankAddress, uint32_t tankSize);
