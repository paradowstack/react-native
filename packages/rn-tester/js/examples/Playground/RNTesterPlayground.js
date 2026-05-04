import * as React from 'react';
import {Animated, Image, StyleSheet, Text, TextInput, View} from 'react-native';

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
    outputRange: ['calc(15vw)', 'calc(25vw)'],  
    });
    const animationSmall2= animValue.interpolate({
    inputRange: [0, 1],
    outputRange: ['calc(16px * 2)', 'calc(166px * 2)'],  
    });
    const animationSmall3 = animValue.interpolate({
    inputRange: [0, 1],
    outputRange: ['calc(0.1vw)', 'calc(3vw)'],  
    });
    const animationTiny = animValue.interpolate({
    inputRange: [0, 1],
    outputRange: ['calc(0.1vw)', 'calc(1.8vw)'],  
    });
    const animationSmallNumber = animValue.interpolate({
    inputRange: [0, 1],
    outputRange: ['calc(1)', 'calc(25)'],  
    });

    const animatedBoxShadow = animValue.interpolate({
      inputRange: [0, 1],
      outputRange: [
        'calc(10vw) calc(10vh) calc(5px) 0px grey',
        'calc(25vw) calc(15vh) calc(25px) 0px grey',
      ],
    });

  return (
    <View style={styles.container}>
       <Animated.View
        style={{
          width: 300,
          height: 300,

          experimental_backgroundImage:
            'linear-gradient(' +
              '45deg, ' +
              'rgb(30, 29, 29)  calc(20% + 10px),' +
              'rgb(82, 55, 122) 100%)',
          boxShadow: animatedBoxShadow,
          outlineWidth: 'calc(22px)',
          outlineColor: 'rgba(82, 55, 122, 0.62)',
          opacity: 'calc(0.95)',
          borderWidth: 'calc(1vw + 10px)',
          borderLeftWidth: 'calc(1vw)',
          borderEndWidth: 'calc(1vw)',
          borderTopLeftRadius: animationSmall2,
          borderBottomRightRadius: animationSmall2,
          borderTopRightRadius: 'calc(90%/2)',

          transform: [{ scale: 'calc(75%)' }, 
                      { translateY: animationTiny }, 
                      { translateX: animationTiny },
                    ],
          // filter: [{blur: animationSmall3}, {saturate: animationSmallNumber}],
        }}
      />
       <Animated.Text style={{ 
          opacity: 'calc(0.6)', 
          fontSize: 'calc(20vw)', 
          marginTop: 'calc(40px)',
          letterSpacing: animationTiny,
          color: 'black',
          textShadowColor: "rgb(82, 55, 122)", 
          textShadowRadius: 'calc(2vw)', 
          textShadowOffset: {width: 20, height: 0}
      }}>callstack</Animated.Text>

      <Animated.Image
          style={{ 
            borderRadius: 'calc(50%)', 
            marginTop: 'calc(20px)'
        }}
        source={{
          height: 'calc(20vw)',
          width: 'calc(20vw)',
          uri: 'https://www.facebook.com/ar_effect/external_textures/648609739826677.png',
        }}
        blurRadius={animationTiny}
      />  

      <TextInput
      value='Input'
      style={{
        color: 'rgba(82, 55, 122, 0.7)',
        fontSize: 'calc(30vw)', 
        textShadowRadius: 'calc(50px)', 
        textShadowColor: "black", 
      }}></TextInput>

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
