main()
{
	int a;
	int i;
	int s1,s2,s3;
	i=100;
	a = 0;
	while(i<1000){
		s1=i/100;
		s2=i/10%10;
		s3=i%10;
		if(s1<s2&&s2>s3)
			a=a+1;	
		i=i+1;
	}
}
