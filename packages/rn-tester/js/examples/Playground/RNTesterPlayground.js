import * as React from 'react';
import {Animated, StyleSheet, Text, View} from 'react-native';

function Playground() {
  const animValue = React.useRef(new Animated.Value(0)).current;

  React.useEffect(() => {
    Animated.loop(
      Animated.sequence([
        Animated.timing(animValue, {
          toValue: 1,
          duration: 3000,
          useNativeDriver: false, 
        }),
        Animated.timing(animValue, {
          toValue: 0,
          duration: 3000,
          useNativeDriver: false,
        }),
      ]),
    ).start();
  }, [animValue]);

  const animatedWidth = animValue.interpolate({
    inputRange: [0, 1],
    outputRange: ['calc(70vw)', 'calc(120vw)'],  
    });

    const animatedHeight = animValue.interpolate({
    inputRange: [0, 1],
    outputRange: ['calc(35vh)', 'calc(60vh)'],  
    });
    const animationSmall1 = animValue.interpolate({
    inputRange: [0, 1],
    outputRange: ['calc(10vw)', 'calc(50vw)'],  
    });
    const animationSmall2= animValue.interpolate({
    inputRange: [0, 1],
    outputRange: ['calc(15vw)', 'calc(75vw)'],  
    });
    const animationSmall3 = animValue.interpolate({
    inputRange: [0, 1],
    outputRange: ['calc(5vw)', 'calc(55vw)'],  
    });
    const animationSmallNumber = animValue.interpolate({
    inputRange: [0, 1],
    outputRange: [15, 100],  
    });

    const animatedBoxShadow = animValue.interpolate({
      inputRange: [0, 1],
      outputRange: [
        'calc(10vw) calc(10vh) calc(5px) 0px grey',
        'calc(10vw) calc(10vh) calc(5px) 0px grey',
      ],
    });

  return (
    <View style={styles.container}>
       <Animated.View
        style={{
          width: 300,
          height: 300,

          // experimental_backgroundImage:
          //   'linear-gradient(' +
          //     '45deg, ' +
          //     'rgb(30, 29, 29)  30%,' +
          //     'rgb(82, 55, 122) 100%)',
          // boxShadow: animatedBoxShadow,
          // outlineWidth: 'calc(2vw)',
          // transform: 'scale(calc(9/16))',
          // opacity: 0.88,
          // borderWidth: 'calc(1vw + 10px)',
          // borderLeftWidth: 'calc(3vw)',
          // borderEndWidth: 'calc(3vw)',
          // borderTopStartRadius: animationSmall1,
          // borderBottomEndRadius: animationSmall2,
          // borderTopEndRadius: 'calc(50% + 10px)',

          backgroundColor: 'lightblue',
          // filter: [{dropShadow: 'calc(30px/2) calc(10vw) 4px #4444dd'}],
        }}
      />
      <Text style={{opacity: 'calc(0.2*2)', fontSizeMultiplier: 1.2, fontSize: 'calc(21vw)', letterSpacing: 'calc(1vw)', color: 'black',
         textShadowColor: "red", textShadowRadius: 'calc(12px)', textShadowOffset: {width: 20, height: 0}
      }}>callstack</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    padding: 10,
    alignItems: 'center',
    justifyContent: 'center',
    flex: 1,
    // direction: 'rtl',
  },
});

export default ({
  title: 'Playground',
  name: 'playground',
  description: 'Test out new features and ideas.',
  render: (): React.Node => <Playground />,
}: RNTesterModuleExample);
