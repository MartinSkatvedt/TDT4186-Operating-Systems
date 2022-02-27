# Test documentation

## Testcase 1

1. Schedule alarm
2. Wait for alarm to trigger
3. Check that no zombie processes remain after alarm trigger

### 1. Expected result

Expects that no zombie processes remain after the alarm clock rings. The test should also expect that the alarm triggers at the specified time.

## Testcase 2

1. Schedule alarm
2. Cancel alarm
3. List alarms

### 2. Expected result

Expects that no alarm shows in the list views and that the procesess have been killed, with no zombie remaining. 

## Testcase 3

1. Schedule alarm
2. Wait for alarm to trigger
3. List alarm

### 3. Expected result

Expects that no alarm shows in the list views. The test should also expect that the alarm triggers at the right time and that the sound is played. 

## Testcase 4

1. Schedule _n_ alarms
2. Check that _n_ processes have been spawned
3. List alarms

### 4. Expected result

Expects that _n_ processes have been spawned. It is also expected that every alarm is shown in the list view.