import * as React from 'react';
import {Animated, StyleSheet, View} from 'react-native';

function Playground() {
  return (
    <View style={styles.container}>
      <Animated.View
        style={{
          width: 'calc(300px)',
          height: 'calc(35vh)',

          experimental_backgroundImage:
            'linear-gradient(' +
              '45deg, ' +
              'rgb(30, 29, 29)  calc(15% * 1.234),' +
              'rgb(82, 55, 122) 100%)',
          boxShadow: 'calc(15vw) calc(45vh + 5vh) 0px 0px rgba(0,0,0,0.75)',
          outlineWidth: 'calc(2vw)',
          transform: 'scale(calc(9/16))',
          borderRadius: 'calc(31vw + 10px)',
          opacity: 'calc(0.75)',
        }}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    padding: 10,
    alignItems: 'center',
    justifyContent: 'center',
    flex: 1,
  },
});

export default {
  title: 'Playground',
  name: 'playground',
  description: 'Test out new features and ideas.',
  render: (): React.Node => <Playground />,
} as RNTesterModuleExample;
