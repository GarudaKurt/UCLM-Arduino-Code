class Distance {
  private:
      #define trig D0
      #define ech D8
      long distance, duration;
  public:
    Distance() {
      pinMode(trig, OUTPUT);
      pinMode(ech, INPUT);
    }
    long totalDistance() {
      digitalWrite(trig, LOW);
      delayMicroseconds(2);
      digitalWrite(trig, HIGH);
      delayMicroseconds(10);
      digitalWrite(trig, LOW);

      duration = pulseIn(ech, HIGH);
      distance = duration / 58.2;
      return distance;
    }
};