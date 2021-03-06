#! /bin/sh

file=${1:-input_map.inc}
echo "# generated by $(basename $0)"
cat <<EOF
begin remote
        name            devinput
        bits            16
        eps             30
        aeps            100
        pre_data_bits   16
        pre_data        0x0001
        post_data_bits  32
        post_data       0x00000001
        gap             132799
        toggle_bit      0

        begin codes
EOF

sed --expression="s/^{\"\([^\"]*\)\", \(.*\)},/         \1      \2/" <$file
cat <<EOF
        end codes
end remote
EOF

echo
echo "# generated by $(basename $0) (obsolete 32 bit version)"
cat <<EOF
begin remote
        name            devinput
        bits            16
        eps             30
        aeps            100
        pre_data_bits   16
        pre_data        0x8001
        gap             132799
        toggle_bit      0

        begin codes
EOF

sed --expression="s/^{\"\([^\"]*\)\", \(.*\)},/         \1      \2/" <$file
cat <<EOF
        end codes
end remote
EOF
