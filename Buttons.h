class Buttons {
  private:
    #define food D6
    #define water D7
    #define medicine D3
    bool lastState = HIGH;
  public:
    Buttons() {
      pinMode(food, INPUT_PULLUP);
      pinMode(water, INPUT_PULLUP);
      pinMode(medicine, INPUT_PULLUP);
    }
    bool PressFood() {
      return digitalRead(food) == LOW;
    }
    bool PressWater() {
      return digitalRead(water) == LOW;
    }
    bool PressMedicine() {
      return digitalRead(medicine) == LOW;
    }
};