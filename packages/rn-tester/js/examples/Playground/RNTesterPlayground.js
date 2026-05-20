import * as React from 'react';
import {Animated, Image, Pressable, StyleSheet, Text, TextInput, View} from 'react-native';

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
    outputRange: ['calc(16px * 2)', 'calc(62px * 2)'],  
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
    const animationPlainNumber = animValue.interpolate({
    inputRange: [0, 1],
    outputRange: [2, 10],  
    });
    const animatedColor = animValue.interpolate({
    inputRange: [0, 1],
    outputRange: ['rgba(82, 55, 122, 0.22)', 'rgba(82, 55, 122, 1)'],  
    });
    const animatedBoxShadow = animValue.interpolate({
      inputRange: [0, 1],
      outputRange: [
        'calc(10vw) calc(10vh) calc(5px) 0px grey',
        'calc(25vw) calc(15vh) calc(25px) 0px grey',
      ],
    });

  const [outlineWidth, setOutlineWidth] = React.useState('calc(2vw + 10px)');
  const [boxShadow, setBoxShadow] = React.useState('calc(10vw) calc(10vh) calc(15px) 0px grey');

  return (
    <View style={styles.container}>
       <Animated.View
        style={{
          width: 300,
          height: 300,
          
          experimental_backgroundImage:
            'linear-gradient(' +
              '45deg, ' +
              'rgb(30, 29, 29)  calc(10% + 10px),' +
              'rgb(82, 55, 122) 100%)',
          boxShadow: animatedBoxShadow,
          outlineWidth: animationSmall1,
          outlineColor: 'rgba(82, 55, 122, 0.7)',
          // opacity: 0.66,
          // borderWidth: animationSmall2,
          borderTopEndRadius: animationSmall2,
          borderBottomRightRadius: animationSmall2,
          borderTopRightRadius: 'calc(90px)',

          transform: [{ scale: 'calc(75%)' }, 
                      // { translateY: animationTiny }, 
                      // { translateX: animationTiny },
                    ],
          // filter: [{blur: animationSmall3}],
        }}
      />
       {/* <Animated.Text style={{ 
          opacity: 'calc(0.6)', 
          fontSize: 'calc(20vw)',
          marginTop: 'calc(20px)',
          letterSpacing: animationTiny,
          color: 'black',
          textShadowColor: "rgb(82, 55, 122)", 
          textShadowRadius: 'calc(20vw)', 
          textShadowOffset: {width: 20, height: 0}
      }}>callstack</Animated.Text> */}

       {/* <Pressable onPress={() => {
        setOutlineWidth(outlineWidth === 'calc(2vw + 10px)' ? 'calc(10vw + 20px)' : 'calc(2vw + 10px)');
        setBoxShadow(boxShadow === 'calc(10vw) calc(10vh) calc(15px) 0px grey' ? 'calc(25vw) calc(15vh) calc(25px) 0px grey' : 'calc(10vw) calc(10vh) calc(15px) 0px grey');
      }}> */}

      <Animated.Image
          style={{ 
            borderRadius: 'calc(30%)', 
            marginTop: 'calc(20px)'
        }}
        source={{
          height: 100,
          width: 100,
          uri: 'https://www.facebook.com/ar_effect/external_textures/648609739826677.png',
        }}
        blurRadius={'calc(5px)'}
      />  
      
      {/* </Pressable> */}

      <TextInput
      value='Input'
      style={{
        color: 'rgba(82, 55, 122, 0.7)',
        fontSize: 'calc(20vw)', 
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
  },
});

export default ({
  title: 'Playground',
  name: 'playground',
  description: 'Test out new features and ideas.',
  render: (): React.Node => <Playground />,
}: RNTesterModuleExample);
