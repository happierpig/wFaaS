FROM xfaas-base

COPY vFaaS /vFaaS

# fulfill the structure requirement of proxy

WORKDIR /vFaaS

RUN bash build.sh

# proxy server runs under port 5000
EXPOSE 18000
ENTRYPOINT [ "/bin/bash", "-l", "-c" ]
CMD [ "./proxy_exe" ]
