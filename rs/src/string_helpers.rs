/*
 WebServe (Rust port)
 Copyright 2021-2024 Peter Pearson.

 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ---------
*/

const TOKEN_SET_SEPARATOR : char = ',';

pub fn combine_set_tokens(str1: &str, str2: &str) -> String {
    // for the moment, just append the two strings together, making sure there's a seperating token seperator char between them,
	// but this is likely going to require being made more robust in the future...

    if str1.is_empty() {
        return str2.to_string();
    }

    if str2.is_empty() {
        return str1.to_string();
    }

    let mut combined_string = str1.to_string();
    if !combined_string.ends_with(TOKEN_SET_SEPARATOR) {
        combined_string.push(TOKEN_SET_SEPARATOR);
        combined_string.push(' ');
    }

    combined_string.push_str(str2);
    combined_string
}

pub fn simple_encode_string(input: &str) -> String {
    let mut output_string = String::new();

    for chr in input.chars() {
        match chr {
            ' ' => {
                output_string.push('+');
            }
            '/' => {
                output_string.push_str("%2F");
            }
            _ => {
                output_string.push(chr);
            }
        }
    }

    output_string
}

pub fn simple_decode_string(input: &str) -> String {
    let mut output_string = input.to_string();

    // convert hex-encoding strings...
    let mut start_pos = 0;
    while let Some(pos) = &output_string[start_pos..].find('%') {
        let encoded_str = &output_string[pos+1..pos+3];
        let hex_value = u8::from_str_radix(encoded_str, 16).expect("hex parse error");
        let char_value = char::from(hex_value);
        output_string.replace_range(pos..&(pos+3), &String::from(char_value));
        start_pos = pos + 1;
    }

    // replace '+' with spaces
    output_string = output_string.replace('+', " ");

    output_string
}


#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_combine_set_tokens_1() {
        
        
    }

    #[test]
    fn test_string_decode_1() {
        let start = "once+upon+a+time";

        assert_eq!(simple_decode_string(&start), "once upon a time");
    }

    #[test]
    fn test_string_decode_2() {
        let start = "once+upon%2Fa+time";

        assert_eq!(simple_decode_string(&start), "once upon/a time");
    }


}
